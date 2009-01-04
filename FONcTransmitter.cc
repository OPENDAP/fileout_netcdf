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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <iostream>
#include <fstream>

#include <DataDDS.h>
#include <BaseType.h>

using namespace::libdap ;

#include "FONcTransmitter.h"
#include "FONcTransform.h"
#include <BESInternalError.h>
#include <TheBESKeys.h>
#include <BESDataDDSResponse.h>
#include <BESDataNames.h>

#define FONC_TEMP_DIR "/tmp"
#define DATA_TRANSMITTER "data"

string FONcTransmitter::temp_dir ;

FONcTransmitter::FONcTransmitter()
    : BESBasicTransmitter()
{
    add_method( DATA_TRANSMITTER, FONcTransmitter::send_data ) ;

    if( FONcTransmitter::temp_dir.empty() )
    {
	// Where is the temp directory for creating these files
	bool found = false ;
	string key = "FONc.Tempdir" ;
	FONcTransmitter::temp_dir =
	    TheBESKeys::TheKeys()->get_key( key, found ) ;
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

void
FONcTransmitter::send_data( BESResponseObject *obj,
			    BESDataHandlerInterface &dhi )
{
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj) ;
    DataDDS *dds = bdds->get_dds() ;
    ostream &strm = dhi.get_output_stream() ;
    if( !strm )
    {
	string err = (string)"Output stream is not set, can not return as" ;
	BESInternalError pe( err, __FILE__, __LINE__ ) ;
	throw pe ;
    }

    dhi.first_container() ;

    string ce = dhi.data[POST_CONSTRAINT] ;
    try
    {
	bdds->get_ce().parse_constraint( ce, *dds);
    }
    catch( Error &e )
    {
	ErrorCode ec = e.get_error_code() ;
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

    string dataset_name = dhi.container->access() ;

    // now we need to read the data
    try
    {
	// Handle *functional* constraint expressions specially 
	if( bdds->get_ce().functional_expression() )
	{
	    // This returns a new BaseType, not a pointer to one in the DataDDS
	    // So once the data has been read using this var create a new
	    // DataDDS and add this new var to the it.
	    BaseType *var = bdds->get_ce().eval_function( *dds, dataset_name ) ;
	    if (!var)
		throw Error(unknown_error, "Error calling the CE function.");

	    var->read( ) ;

	    // FIXME: Do I need to delete the other DataDDS? Do I need it
	    // anymore. I've got what I need doing the eval_function call
	    // and I'm going to create a new DataDDS with it. So I don't
	    // think I need the old one.
	    //
	    // delete dds ;
	    dds = new DataDDS( NULL, "virtual" ) ;
	    dds->add_var( var ) ;
	}
	else
	{
	    // Iterate through the variables in the DataDDS and read in the data
	    // if the variable has the send flag set. 

	    // FIXME: A problem here is that the netcdf handler really
	    // expects there to only be one container. This is a problem.

            // Note the special case for Sequence. The transfer_data() method
            // uses the same logic as serialize() to read values but transfers
            // them to the d_values field instead of writing them to a XDR sink
            // pointer. jhrg 9/13/06
	    for( DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++ )
	    {
		if( (*i)->send_p() )
		{
		    // FIXME: we don't have sequences in netcdf so let's not
		    // worry about that right now.
		    (*i)->read( ) ;
		}
	    }
	}
    }
    catch( Error &e )
    {
	ErrorCode ec = e.get_error_code() ;
	string em = e.get_error_message() ;
	string err = "Failed to read data: " + em ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    catch( ... )
    {
	string err = "Failed to read data: Unknown exception caught" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    char *temp_name = 0 ;
    char *nc_temp = "ncXXXXXX" ;
#if defined(WIN32) || defined(TEST_WIN32_TEMPS)
    temp_name = _mktemp( nc_temp ) ;
#else
    temp_name = mktemp( nc_temp ) ;
#endif
    if( !temp_name )
    {
	string s = (string)"File out netcdf, "
		   + "was not able to generate temporary file name." ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
    string temp_full = FONcTransmitter::temp_dir + "/" + temp_name ;

    FONcTransform ft( dds, temp_full ) ;
    ft.transform() ;
    FONcTransmitter::return_temp_stream( temp_full, strm ) ;
    if( !access( temp_full.c_str(), F_OK ) )
    {
	remove( temp_full.c_str() ) ;
    }
}

void
FONcTransmitter::return_temp_stream( const string &filename,
				     ostream &strm )
{
    int bytes = 0 ;
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
	if( TheBESKeys::TheKeys()->get_key( "BES.Communication.Protocol", found ) == "HTTP" )
	{
	    cout << "HTTP/1.0 200 OK\n" ;
	    cout << "Content-type: application/octet-stream\n" ;
	    cout << "Content-Description: " << "BES dataset" << "\n\n" ;
	    cout << flush ;
	}
	strm.write( block, nbytes ) ;
	bytes += nbytes ;
    }
    else
    {
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
	bytes += nbytes ;
    }
    os.close();
    // debug line control how many bytes were sent vs. the size of the target file
    // cerr<<__FILE__<<":"<<__LINE__<<": wrote "<<bytes<<" in the return stream"<<endl;
}

