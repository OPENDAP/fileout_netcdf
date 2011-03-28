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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include <DataDDS.h>
#include <BaseType.h>
#include <escaping.h>

using namespace::libdap ;

#include "FONcTransmitter.h"
#include "FONcTransform.h"
#include <BESInternalError.h>
#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#define FONC_TEMP_DIR "/tmp"

string FONcTransmitter::temp_dir ;

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
FONcTransmitter::FONcTransmitter()
    : BESBasicTransmitter()
{
    add_method( DATA_SERVICE, FONcTransmitter::send_data ) ;

    if( FONcTransmitter::temp_dir.empty() )
    {
	// Where is the temp directory for creating these files
	bool found = false ;
	string key = "FONc.Tempdir" ;
	TheBESKeys::TheKeys()->get_value( key,
					  FONcTransmitter::temp_dir, found ) ;
	if( !found || FONcTransmitter::temp_dir.empty() )
	{
	    FONcTransmitter::temp_dir = FONC_TEMP_DIR ;
	}
	string::size_type len = FONcTransmitter::temp_dir.length() ;
	if( FONcTransmitter::temp_dir[len - 1] == '/' )
	{
	    FONcTransmitter::temp_dir =
		FONcTransmitter::temp_dir.substr( 0, len- 1 ) ;
	}
    }
}

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
void
FONcTransmitter::send_data( BESResponseObject *obj,
			    BESDataHandlerInterface &dhi )
{
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj) ;
    if( !bdds )
    {
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;
    }

    DataDDS *dds = bdds->get_dds() ;
    ostream &strm = dhi.get_output_stream() ;
    if( !strm )
    {
	string err = (string)"Output stream is not set, can not return as" ;
	BESInternalError pe( err, __FILE__, __LINE__ ) ;
	throw pe ;
    }

    BESDEBUG( "fonc",
	      "FONcTransmitter::send_data - parsing the constraint" << endl ) ;

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");
    try
    {
	bdds->get_ce().parse_constraint( ce, *dds);
    }
    catch( Error &e )
    {
	string em = e.get_error_message() ;
	string err = "Failed to parse the constraint expression: " + em ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    catch( ... )
    {
	string err = (string)"Failed to parse the constraint expression: "
	             + "Unknown exception caught" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    // The dataset_name is no longer used in the constraint evaluator, so no
    // need to get here. Plus, just getting the first containers dataset
    // name would not have worked with multiple containers.
    // pwest Jan 4, 2009
    string dataset_name = "" ;

    // now we need to read the data
    BESDEBUG( "fonc", "FONcTransmitter::send_data - reading data into DataDDS"
		      << endl ) ;
    // This is used to record whetehr this is a functional CE or not. If so,
    // the code allocates a new DDS object to hold the BaseType returned by
    // the function and we need to delete that DDS before exiting this code.
    bool functional_constraint = false;
    try
    {
	// Handle *functional* constraint expressions specially
	if (bdds->get_ce().function_clauses())
	{
	    BESDEBUG( "fonc", "processing a functional constraint clause(s)." << endl );
	    dds = bdds->get_ce().eval_function_clauses(*dds);
	}
#if 0
	if( bdds->get_ce().functional_expression() )
	{
	    // This returns a new BaseType, not a pointer to one in the DataDDS
	    // So once the data has been read using this var create a new
	    // DataDDS and add this new var to the it.
	    BaseType *var = bdds->get_ce().eval_function( *dds, dataset_name ) ;
	    if (!var)
		throw Error(unknown_error, "Error calling the CE function.");

	    var->read( ) ;

	    dds = new DataDDS( NULL, "virtual" ) ;
	    // Set 'functional_constraint' here so that below we know that if
	    // it's true we must delete 'dds'.
	    functional_constraint = true;
	    dds->add_var( var ) ;
	}
#endif
	else
	{
	    // Iterate through the variables in the DataDDS and read
	    // in the data if the variable has the send flag set.

            // Note the special case for Sequence. The
	    // transfer_data() method uses the same logic as
	    // serialize() to read values but transfers them to the
	    // d_values field instead of writing them to a XDR sink
	    // pointer. jhrg 9/13/06
	    for( DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++ )
	    {
		if( (*i)->send_p() )
		{
		    // FIXME: we don't have sequences in netcdf so let's not
		    // worry about that right now.
                    (*i)->intern_data(bdds->get_ce(), *dds);
		}
	    }
	}
    }
    catch( Error &e )
    {
        if (functional_constraint)
            delete dds;
	string em = e.get_error_message() ;
	string err = "Failed to read data: " + em ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    catch( ... )
    {
        if (functional_constraint)
            delete dds;
	string err = "Failed to read data: Unknown exception caught" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    string temp_file_name = FONcTransmitter::temp_dir + '/' + "ncXXXXXX";
    char *temp_full = new char[temp_file_name.length() + 1];
    string::size_type len = temp_file_name.copy(temp_full, temp_file_name.length());
    *(temp_full + len) = '\0';
    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask( 077 );
    int fd = mkstemp( temp_full ) ;
    umask( original_mode );

    if( fd == -1 )
    {
        delete [] temp_full;
        if (functional_constraint)
            delete dds;
        string err = string("Failed to open the temporary file: ")
		     + temp_file_name ;
        throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    // transform the OPeNDAP DataDDS to the netcdf file
    BESDEBUG( "fonc",
	      "FONcTransmitter::send_data - transforming into temporary file "
	      << temp_full << endl ) ;
    try
    {
	FONcTransform ft( dds, dhi, temp_full ) ;
	ft.transform() ;

	BESDEBUG( "fonc",
		  "FONcTransmitter::send_data - transmitting temp file "
		  << temp_full << endl ) ;
	FONcTransmitter::return_temp_stream( temp_full, strm ) ;
    }
    catch( BESError &e )
    {
        close(fd);
	(void)unlink( temp_full ) ;
	delete[] temp_full;
	if (functional_constraint)
	    delete dds;
	throw;
    }
    catch( ... )
    {
        close(fd);
	(void)unlink( temp_full ) ;
	delete[] temp_full;
        if (functional_constraint)
            delete dds;
	string err = (string)"File out netcdf, "
		     + "was not able to transform to netcdf, unknown error" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    close(fd);
    (void)unlink( temp_full ) ;
    delete[] temp_full;
    if (functional_constraint)
        delete dds;
    BESDEBUG( "fonc",
	      "FONcTransmitter::send_data - done transmitting to netcdf"
	      << endl ) ;
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
void
FONcTransmitter::return_temp_stream( const string &filename,
				     ostream &strm )
{
    //  int bytes = 0 ;    // Not used; jhrg 3/16/11
    ifstream os ;
    os.open( filename.c_str(), ios::binary|ios::in ) ;
    if( !os )
    {
	string err = "Can not connect to file " + filename ;
	BESInternalError pe(err, __FILE__, __LINE__ ) ;
	throw pe ;
    }
    int nbytes ;
    char block[4096] ;

    os.read( block, sizeof block ) ;
    nbytes = os.gcount() ;
    if( nbytes > 0 )
    {
	bool found = false ;
	string context = "transmit_protocol" ;
	string protocol =
	    BESContextManager::TheManager()->get_context( context, found ) ;
	if( protocol == "HTTP" )
	{
	    strm << "HTTP/1.0 200 OK\n" ;
	    strm << "Content-type: application/octet-stream\n" ;
	    strm << "Content-Description: " << "BES dataset" << "\n" ;
	    strm << "Content-Disposition: filename=" << filename << ".nc;\n\n" ;
	    strm << flush ;
	}
	strm.write( block, nbytes ) ;
	//bytes += nbytes ;
    }
    else
    {
	// close the stream before we leave.
	os.close() ;

	string err = (string)"0XAAE234F: failed to stream. Internal server "
	             + "error, got zero count on stream buffer." + filename ;
	BESInternalError pe( err, __FILE__, __LINE__ ) ;
	throw pe ;
    }
    while (os)
    {
	os.read( block, sizeof block ) ;
	nbytes = os.gcount() ;
	strm.write( block, nbytes ) ;
	//write( fileno( stdout ),(void*)block, nbytes ) ;
	//bytes += nbytes ;
    }
    os.close();
}

