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

#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include <DataDDS.h>
#include <BaseType.h>

using namespace::libdap ;

#include "FONcTransmitter.h"
#include "FONcGridTransform.h"
#include <BESInternalError.h>
#include <TheBESKeys.h>
#include <BESDataDDSResponse.h>
#include <BESDatanames.h>
#define NETCDF_TEMP_STREAM "temp_netcdf_file"
#define DATA_TRANSMITTER "data"

FONcTransmitter::FONcTransmitter()
    : BESBasicTransmitter()
{
    add_method( DATA_TRANSMITTER, FONcTransmitter::send_data ) ;
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

    FONcGridTransform::create_local_nc( dds, NETCDF_TEMP_STREAM ) ;
    dhi.first_container() ;
    while( dhi.container )
    {
	add_attributes( dhi.container, NETCDF_TEMP_STREAM ) ;
	dhi.next_container() ;
    }
    FONcTransmitter::return_temp_stream( NETCDF_TEMP_STREAM, strm ) ;
    if( !access( NETCDF_TEMP_STREAM, F_OK ) )
    {
	remove( NETCDF_TEMP_STREAM ) ;
    }
}

void
FONcTransmitter::add_attributes( const BESContainer *c,
                                  const string &filename )
{
    string attributes =c->get_attributes() ;
    if( attributes != "" )
    {
	string variable ;
	if( attributes[attributes.size()-1] == ',' )
	{
	    string err = "attributes " + attributes
			 + " can not end with a comma (,)" ;
	    BESInternalError e( err, __FILE__, __LINE__ ) ;
	    throw e ;
	}

	unsigned int pos = attributes.find( "," ) ;
	while( pos != string::npos )
	{
	    variable = attributes.substr( 0, pos ) ;
	    attributes = attributes.substr( pos+1, attributes.size() ) ;
	    FONcGridTransform::copy_all_attributes( variable,
	    					     c->get_real_name(),
						     filename ) ;
	    //cout << c->get_real_name() << " " << variable << endl ;
	    pos = attributes.find( "," ) ;
	}
	if( attributes != "" )
	{
	    variable = attributes ;
	    FONcGridTransform::copy_all_attributes( variable,
	    					     c->get_real_name(),
						     filename ) ;
	    //cout << c->get_real_name() << " " << variable << endl ;
	}
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
	//write( fileno( stdout ),(void*)block, nbytes ) ;
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

