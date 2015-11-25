// FONcTransmitter.cc

// This file is part of BES Netcdf File Out Module

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <exception>

#include <DataDDS.h>
#include <BaseType.h>
#include <escaping.h>
#include <ConstraintEvaluator.h>

#include <BESInternalError.h>
#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include "FONcRequestHandler.h"
#include "FONcTransmitter.h"
#include "FONcTransform.h"

using namespace ::libdap;
using namespace std;

//#define FONC_TEMP_DIR "/tmp"
// size of the buffer used to read from the temporary file built on disk and
// send data to the client over the network connection (socket/stream)
#define BLOCK_SIZE 4096

#define RETURNAS_NETCDF "netcdf"
#define RETURNAS_NETCDF4 "netcdf-4"

//string FONcTransmitter::temp_dir;

/** @brief Construct the FONcTransmitter, adding it with name netcdf to be
 * able to transmit a data response
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as a netcdf file.
 *
 * The OPeNDAP data object is written to a netcdf file locally in a
 * temporary directory specified by the BES configuration parameter
 * FONc.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FONC_TEMP_DIR.
 */
FONcTransmitter::FONcTransmitter() :
    BESBasicTransmitter()
{
    add_method(DATA_SERVICE, FONcTransmitter::send_data);
#if 0
    if (FONcTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "FONc.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, FONcTransmitter::temp_dir, found);
        if (!found || FONcTransmitter::temp_dir.empty()) {
            FONcTransmitter::temp_dir = FONC_TEMP_DIR;
        }
        string::size_type len = FONcTransmitter::temp_dir.length();
        if (FONcTransmitter::temp_dir[len - 1] == '/') {
            FONcTransmitter::temp_dir = FONcTransmitter::temp_dir.substr(0, len - 1);
        }
    }
#endif
}

#if 0
/**
 * Hack to ensure the file descriptor for the temporary file is closed.
 */
struct wrap_file_descriptor {
    int fd;
    wrap_file_descriptor(int i) : fd(i) { }
    ~wrap_file_descriptor() { cerr << "*** Closing fd" << endl; close(fd); }
};
#endif

/** @brief The static method registered to transmit OPeNDAP data objects as
 * a netcdf file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a netcdf file, and
 * streams back that netcdf file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a netcdf file, or
 * streaming the netcdf file
 */
void FONcTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds) throw BESInternalError("cast error", __FILE__, __LINE__);

    DataDDS *dds = bdds->get_dds();
    if (!dds) throw BESInternalError("No DataDDS has been created for transmit", __FILE__, __LINE__);

    BESDEBUG("fonc", "FONcTransmitter::send_data - parsing the constraint" << endl);

    ConstraintEvaluator &eval = bdds->get_ce();

    string ncVersion = dhi.data[RETURN_CMD];

    ostream &strm = dhi.get_output_stream();
    if (!strm) throw BESInternalError("Output stream is not set, can not return as", __FILE__, __LINE__);

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");
    try {
        eval.parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        throw BESInternalError("Failed to parse the constraint expression: " + e.get_error_message(), __FILE__,
            __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to parse the constraint expression: Unknown exception caught", __FILE__,
            __LINE__);
    }

    // The dataset_name is no longer used in the constraint evaluator, so no
    // need to get here. Plus, just getting the first container's dataset
    // name would not have worked with multiple containers.
    // pwest Jan 4, 2009
    // string dataset_name = "";

    // now we need to read the data
    BESDEBUG("fonc", "FONcTransmitter::send_data - reading data into DataDDS" << endl);

    // ADB: remember when we're using a temp DDS
    // bool using_temp_dds = false; See comment below about set_dds(). jhrg 8/8/14

    try {
        // Handle *functional* constraint expressions specially
        if (eval.function_clauses()) {
            BESDEBUG("fonc", "processing a functional constraint clause(s)." << endl);
            DataDDS *tmp_dds = eval.eval_function_clauses(*dds);
            // This fixes the issue Aron (ADB) reported. jhrg 8/8/14
            bdds->set_dds(tmp_dds);
            delete dds;
            dds = tmp_dds;
            // ADB: don't delete DDS here because it will already be
            // deleted by ~BESDataDDSResponse().
            // delete dds;

            // ADB: instead, remember that you're using a temp and
            // delete it later
            //using_temp_dds = true;
        }
        else {
            // Iterate through the variables in the DataDDS and read
            // in the data if the variable has the send flag set.

            for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
                if ((*i)->send_p()) {
                    (*i)->intern_data(eval, *dds);
                }
            }
        }
    }
#if 0
    catch (BESError &e) {
        throw e;
    }
    catch (Error &e) {
        throw BESInternalError("Failed to read data: " + e.get_error_message(), __FILE__, __LINE__);
    }
#endif
    catch (std::exception &e) {
        throw BESInternalError("Failed to read data: STL Error: " + string(e.what()), __FILE__, __LINE__);
    }
#if 0
    catch (...) {
        throw BESInternalError("Failed to read data: Unknown exception caught", __FILE__, __LINE__);
    }
#endif

    //string temp_file_name = FONcTransmitter::temp_dir + '/' + "ncXXXXXX";
    string temp_file_name = FONcRequestHandler::temp_dir + '/' + "ncXXXXXX";
    vector<char> temp_full(temp_file_name.length() + 1);
    string::size_type len = temp_file_name.copy(&temp_full[0], temp_file_name.length());
    temp_full[len] = '\0';
    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    int fd = mkstemp(&temp_full[0]);
    umask(original_mode);
#if 0
    // Trick: We can unlink the file right here - it will persist until the file descriptor
    // is closed. This means we don't have to litter the code with call to unlink(). jhrg 11/25/15
    (void) unlink(&temp_full[0]);
    // Hack: Use this simple class to 'wrap' the file descriptor so that we can be sure it
    // is closed no matter how this code is exited. jhrg 11/25/15
    wrap_file_descriptor wrapped_fd(fd);
#endif
    if (fd == -1) throw BESInternalError("Failed to open the temporary file: " + temp_file_name, __FILE__, __LINE__);

    // transform the OPeNDAP DataDDS to the netcdf file
    BESDEBUG("fonc", "FONcTransmitter::send_data - transforming into temporary file " << &temp_full[0] << endl);

    try {
        FONcTransform ft(dds, dhi, &temp_full[0], ncVersion);
        ft.transform();

        BESDEBUG("fonc", "FONcTransmitter::send_data - transmitting temp file " << &temp_full[0] << endl);
        FONcTransmitter::return_temp_stream(&temp_full[0], strm, ncVersion);
    }
#if 0
    catch (BESError &e) {
        close(fd);
        throw;
    }
#endif
    catch (std::exception &e) {
        (void) unlink(&temp_full[0]);
        close(fd);
        throw BESInternalError("Failed to read data: STL Error: " + string(e.what()), __FILE__, __LINE__);
    }
#if 0
    catch (...) {
        close(fd);
        throw BESInternalError("File out netcdf, was not able to transform to netcdf, unknown error", __FILE__, __LINE__);
    }

    close(fd);  // The 'wrap_file_descriptor' struct will close this.
#endif

    (void) unlink(&temp_full[0]);
    close(fd);

    BESDEBUG("fonc", "FONcTransmitter::send_data - done transmitting to netcdf" << endl);
}

/** @brief stream the temporary netcdf file back to the requester
 *
 * Streams the temporary netcdf file specified by filename to the specified
 * C++ ostream
 *
 * @param filename The name of the file to stream back to the requester
 * @param strm C++ ostream to write the contents of the file to
 * @throws BESInternalError if problem opening the file
 */
void FONcTransmitter::return_temp_stream(const string &filename, ostream &strm, const string &ncVersion)
{
    ifstream os;
    os.open(filename.c_str(), ios::binary | ios::in);
    if (!os)
        throw BESInternalError("Fileout netcdf: Cannot connect to netcdf file.", __FILE__, __LINE__);;

    char block[BLOCK_SIZE];

    os.read(block, sizeof block);
    int nbytes = os.gcount();
    if (nbytes > 0) {
        bool found = false;
        string context = "transmit_protocol";
        string protocol = BESContextManager::TheManager()->get_context(context, found);
        if (protocol == "HTTP") {
            strm << "HTTP/1.0 200 OK\n";
            strm << "Content-type: application/octet-stream\n";
            strm << "Content-Description: " << "BES dataset" << "\n";
            if (ncVersion == RETURNAS_NETCDF4) {
                strm << "Content-Disposition: filename=" << filename << ".nc4;\n\n";
            }
            else {
                strm << "Content-Disposition: filename=" << filename << ".nc;\n\n";
            }
            strm << flush;
        }
        strm.write(block, nbytes);
    }
    else {
        // close the stream before we leave.
        os.close();
        throw BESInternalError("Fileout netcdf: Failed to stream the response back to the client, got zero count on stream buffer.", __FILE__, __LINE__);
    }

    while (os) {
        os.read(block, sizeof block);
        strm.write(block, os.gcount());
    }

    os.close();
}

