/*
 * test_send_data.cc
 *
 *  Created on: Feb 3, 2013
 *      Author: jimg
 */

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <DataDDS.h>
#include <ConstraintEvaluator.h>
#include <escaping.h>

#include <BESInternalError.h>
#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include <BESDataHandlerInterface.h>

#include "FONcTransform.h"

using namespace ::libdap;

string temp_dir = "/tmp";

using namespace libdap;


/** @brief stream the temporary netcdf file back to the requester
 *
 * @note This is a copy of the code in FONcTransmitter
 *
 * Streams the temporary netcdf file specified by filename to the specified
 * C++ ostream
 *
 * @param filename The name of the file to stream back to the requester
 * @param strm C++ ostream to write the contents of the file to
 * @throws BESInternalError if problem opening the file
 */
static void return_temp_stream(const string &filename, ostream &strm)
{
    //  int bytes = 0 ;    // Not used; jhrg 3/16/11
    ifstream os;
    os.open(filename.c_str(), ios::binary | ios::in);
    if (!os) {
        string err = "Can not connect to file " + filename;
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }
    int nbytes;
    char block[4096];

    os.read(block, sizeof block);
    nbytes = os.gcount();
    if (nbytes > 0) {
        bool found = false;
        string context = "transmit_protocol";
        string protocol = BESContextManager::TheManager()->get_context(context, found);
        if (protocol == "HTTP") {
            strm << "HTTP/1.0 200 OK\n";
            strm << "Content-type: application/octet-stream\n";
            strm << "Content-Description: " << "BES dataset" << "\n";
            strm << "Content-Disposition: filename=" << filename << ".nc;\n\n";
            strm << flush;
        }
        strm.write(block, nbytes);
        //bytes += nbytes ;
    }
    else {
        // close the stream before we leave.
        os.close();

        string err = (string) "0XAAE234F: failed to stream. Internal server "
                + "error, got zero count on stream buffer." + filename;
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }
    while (os) {
        os.read(block, sizeof block);
        nbytes = os.gcount();
        strm.write(block, nbytes);
        //write( fileno( stdout ),(void*)block, nbytes ) ;
        //bytes += nbytes ;
    }
    os.close();
}

/** @brief Test version of FONcTransmitter method
 *
 * This version of send_data() does not use the DAP code that's now a BES
 * module (and not a library, which is what this code used to link with).
 */
void send_data(DataDDS *dds, ConstraintEvaluator &eval, BESDataHandlerInterface &dhi)
{
    ostream &strm = dhi.get_output_stream();
    if (!strm) {
        string err = (string) "Output stream is not set, can not return as";
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");
    try {
        eval.parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        string em = e.get_error_message();
        string err = "Failed to parse the constraint expression: " + em;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    catch (...) {
        string err = (string) "Failed to parse the constraint expression: " + "Unknown exception caught";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // The dataset_name is no longer used in the constraint evaluator, so no
    // need to get here. Plus, just getting the first containers dataset
    // name would not have worked with multiple containers.
    // pwest Jan 4, 2009
    string dataset_name = "";

    // now we need to read the data
    BESDEBUG("fonc", "FONcTransmitter::send_data - reading data into DataDDS" << endl);

    // I removed the functional_constraint bool and the (dead) code that used it.
    // This kind of temporary object should use auto_ptr<>, but in this case it
    // seems like it's not a supported feature of the handler. 12.27.2011 jhrg

    try {
        // Handle *functional* constraint expressions specially
        if (eval.function_clauses()) {
            BESDEBUG("fonc", "processing a functional constraint clause(s)." << endl);
            dds = eval.eval_function_clauses(*dds);
        }
        else
        {
            // Iterate through the variables in the DataDDS and read
            // in the data if the variable has the send flag set.

            // Note the special case for Sequence. The
            // transfer_data() method uses the same logic as
            // serialize() to read values but transfers them to the
            // d_values field instead of writing them to a XDR sink
            // pointer. jhrg 9/13/06
            for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
                if ((*i)->send_p()) {
                    // FIXME: we don't have sequences in netcdf so let's not
                    // worry about that right now.
                    (*i)->intern_data(eval, *dds);
                }
            }
        }
    }
    catch (Error &e) {
        string em = e.get_error_message();
        string err = "Failed to read data: " + em;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    catch (...) {
        string err = "Failed to read data: Unknown exception caught";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    string temp_file_name = temp_dir + '/' + "ncXXXXXX";
    char *temp_full = new char[temp_file_name.length() + 1];
    string::size_type len = temp_file_name.copy(temp_full, temp_file_name.length());
    *(temp_full + len) = '\0';
    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    int fd = mkstemp(temp_full);
    umask(original_mode);

    if (fd == -1) {
        delete[] temp_full;
        string err = string("Failed to open the temporary file: ") + temp_file_name;
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // transform the OPeNDAP DataDDS to the netcdf file
    BESDEBUG("fonc", "FONcTransmitter::send_data - transforming into temporary file " << temp_full << endl);
    try {
    	// this ctor defaults to netcdf 3 output
        FONcTransform ft(dds, dhi, temp_full);
        ft.transform();

        BESDEBUG("fonc", "FONcTransmitter::send_data - transmitting temp file " << temp_full << endl);
        return_temp_stream(temp_full, strm);
    }
    catch (BESError &e) {
        close(fd);
        (void) unlink(temp_full);
        delete[] temp_full;
        throw;
    }
    catch (...) {
        close(fd);
        (void) unlink(temp_full);
        delete[] temp_full;
        string err = (string) "File out netcdf, " + "was not able to transform to netcdf, unknown error";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    close(fd);
    (void) unlink(temp_full);
    delete[] temp_full;
    BESDEBUG("fonc", "FONcTransmitter::send_data - done transmitting to netcdf" << endl);
}




