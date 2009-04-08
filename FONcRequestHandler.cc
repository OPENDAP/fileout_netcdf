// FONcRequestHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

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


#include "FONcRequestHandler.h"
#include <BESResponseHandler.h>
#include <BESResponseNames.h>
#include <BESVersionInfo.h>
#include <BESDataNames.h>
#include <BESDataNames.h>
#include <TheBESKeys.h>
#include "config.h"

FONcRequestHandler::FONcRequestHandler( const string &name )
    : BESRequestHandler( name )
{
    add_handler( HELP_RESPONSE, FONcRequestHandler::build_help ) ;
    add_handler( VERS_RESPONSE, FONcRequestHandler::build_version ) ;
}

FONcRequestHandler::~FONcRequestHandler()
{
}

bool
FONcRequestHandler::build_help( BESDataHandlerInterface &dhi )
{
    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESInfo *info = dynamic_cast < BESInfo * >(response) ;
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    bool found = false ;
    string key = "FONc.Reference" ;
    string ref = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( ref.empty() )
	ref = "http://docs.opendap.org/index.php/BES_-_Modules_-_FileOut_Netcdf" ;
    map<string,string> attrs ;
    attrs["name"] = PACKAGE_NAME ;
    attrs["version"] = PACKAGE_VERSION ;
    attrs["reference"] = ref ;
    info->begin_tag( "module", &attrs ) ;
    info->end_tag( "module" ) ;

    return true ;
}

bool
FONcRequestHandler::build_version( BESDataHandlerInterface &dhi )
{
    BESResponseObject *response = dhi.response_handler->get_response_object() ;
    BESVersionInfo *info = dynamic_cast < BESVersionInfo * >(response) ;
    if( !info )
	throw BESInternalError( "cast error", __FILE__, __LINE__ ) ;

    info->add_module( PACKAGE_NAME, PACKAGE_VERSION ) ;

    return true ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcRequestHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcRequestHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESRequestHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

