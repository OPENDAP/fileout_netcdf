// FONcModule.cc

// This file is part of BES filout NetCDF Module.

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

#include <iostream>

using std::endl ;

#include "FONcModule.h"
#include "FONcTransmitter.h"
#include "FONcRequestHandler.h"
#include "BESRequestHandlerList.h"

#include <BESReturnManager.h>

#include <BESServiceRegistry.h>
#include <BESDapNames.h>

#include <TheBESKeys.h>
#include <BESDebug.h>

#define RETURNAS_NETCDF "netcdf"
#define RETURNAS_NETCDF4 "netcdf-4"

/** @brief initialize the module by adding callbacks and registering
 * objects with the framework
 *
 * Registers the request handler to add to a version or help request,
 * and adds the File Out transmitter for a "returnAs netcdf" request.
 * Also adds netcdf as a return for the dap service dods request and
 * registers the debug context.
 *
 * @param modname The name of the module being loaded
 */
void
FONcModule::initialize( const string &modname )
{
    BESDEBUG( "fonc", "Initializing module " << modname << endl ) ;

    BESDEBUG( "fonc", "    adding " << modname << " request handler" << endl ) ;
    BESRequestHandler *handler = new FONcRequestHandler( modname ) ;
    BESRequestHandlerList::TheList()->add_handler( modname, handler ) ;

    BESDEBUG( "fonc", "    adding " << RETURNAS_NETCDF << " transmitter"
		     << endl ) ;
    BESReturnManager::TheManager()->add_transmitter( RETURNAS_NETCDF,
						     new FONcTransmitter( ) ) ;

    BESDEBUG( "fonc", "    adding fonc netcdf service to dap" << endl ) ;
    BESServiceRegistry::TheRegistry()->add_format( OPENDAP_SERVICE,
						   DATA_SERVICE,
						   RETURNAS_NETCDF ) ;

    BESDEBUG( "fonc", "    adding " << RETURNAS_NETCDF4 << " transmitter"
    		<< endl ) ;
    BESReturnManager::TheManager()->add_transmitter( RETURNAS_NETCDF4,
							new FONcTransmitter( ) ) ;

    BESDEBUG( "fonc", "    adding fonc netcdf4 service to dap" << endl ) ;
    BESServiceRegistry::TheRegistry()->add_format( OPENDAP_SERVICE,
							DATA_SERVICE,
							RETURNAS_NETCDF4 ) ;


    BESDEBUG( "fonc", "    adding fonc debug context" << endl ) ;
    BESDebug::Register( "fonc" ) ;

    BESDEBUG( "fonc", "Done Initializing module " << modname << endl ) ;
}

/** @brief removes any registered callbacks or objects from the
 * framework
 *
 * Any registered callbacks or objects, registered during
 * initialization, are removed from the framework.
 *
 * @param modname The name of the module being removed
 */
void
FONcModule::terminate( const string &modname )
{
    BESDEBUG( "fonc", "Cleaning module " << modname << endl ) ;

    BESDEBUG( "fonc", "    removing " << RETURNAS_NETCDF << " transmitter"
		     << endl ) ;
    BESReturnManager::TheManager()->del_transmitter( RETURNAS_NETCDF ) ;

    BESDEBUG( "fonc", "    removing " << RETURNAS_NETCDF4 << " transmitter "
    		      << endl ) ;

	BESReturnManager::TheManager()->del_transmitter( RETURNAS_NETCDF4 ) ;

    BESDEBUG( "fonc", "    removing " << modname << " request handler "
    		<< endl ) ;
    BESRequestHandler *rh =
	BESRequestHandlerList::TheList()->remove_handler( modname ) ;
    if( rh ) delete rh ;

    BESDEBUG( "fonc", "Done Cleaning module " << modname << endl ) ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus any instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcModule::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcModule::dump - ("
        << (void *) this << ")" << endl;
}

/** @brief A c function that adds this module to the list of modules to
 * be dynamically loaded.
 */
extern "C"
{
    BESAbstractModule *maker()
    {
	return new FONcModule ;
    }
}

