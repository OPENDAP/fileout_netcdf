// FONcTransform.cc

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

#include <sstream>

using std::ostringstream ;
using std::istringstream ;

#include "FONcTransform.h"
#include "FONcUtils.h"
#include "FONcBaseType.h"
#include "FONcAttributes.h"

#include <DDS.h>
#include <Structure.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <BESDebug.h>
#include <BESInternalError.h>

#define ATTRIBUTE_SEPARATOR "."
#define FONC_ORIGINAL_NAME "fonc_original_name"

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FONcTransform::FONcTransform( DDS *dds, BESDataHandlerInterface &dhi,
			      const string &localfile )
    : _ncid( 0 ), _dds( 0 )
{
    if( !dds )
    {
	string s = (string)"File out netcdf, "
		   + "null DDS passed to constructor" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
    if( localfile.empty() )
    {
	string s = (string)"File out netcdf, "
		   + "empty local file name passed to constructor" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
    _localfile = localfile ;
    _dds = dds ;

    // if there is a variable, attribute, dimension name that is not
    // compliant with netcdf naming conventions then we will create
    // a new name. If the new name does not begin with an alpha
    // character then we will prefix it with name_prefix. We will
    // get this prefix from the type of data that we are reading in,
    // such as nc, h4, h5, ff, jg, etc...
    dhi.first_container() ;
    if( dhi.container )
    {
	FONcUtils::name_prefix = dhi.container->get_container_type() + "_" ;
    }
    else
    {
	FONcUtils::name_prefix = "nc_" ;
    }
}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
FONcTransform::~FONcTransform()
{
    bool done = false ;
    while( !done )
    {
	vector<FONcBaseType *>::iterator i = _fonc_vars.begin() ;
	vector<FONcBaseType *>::iterator e = _fonc_vars.end() ;
	if( i == e )
	{
	    done = true ;
	}
	else
	{
	    // These are the FONc types, not the actual ones
	    FONcBaseType *b = (*i) ;
	    delete b ;
	    _fonc_vars.erase( i ) ;
	}
    }
}

/** @brief Transforms each of the varaibles of the DataDDS to the NetCDF
 * file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
void
FONcTransform::transform( )
{
    FONcUtils::reset() ;

    // Convert the DDS into an internal format to keep track of
    // varaibles, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.
    DDS::Vars_iter vi = _dds->var_begin() ;
    DDS::Vars_iter ve = _dds->var_end() ;
    for( ; vi != ve; vi++ )
    {
	if( (*vi)->send_p() )
	{
	    BaseType *v = *vi ;
	    BESDEBUG( "fonc", "converting " << v->name() << endl ) ;
	    FONcBaseType *fb = FONcUtils::convert( v ) ;
	    _fonc_vars.push_back( fb ) ;
	    vector<string> embed ;
	    fb->convert( embed ) ;
	}
    }
    BESDEBUG( "fonc", *this << endl ) ;

    // Open the file for writing
    int stax = nc_create( _localfile.c_str(), NC_CLOBBER, &_ncid ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"File out netcdf, "
	             + "unable to open file " + _localfile ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }

    try
    {
	// Here we will be defining the variables of the netcdf and
	// adding attributes. To do this we must be in define mode.
	nc_redef( _ncid ) ;

	// For each conerted FONc object, call define on it to define
	// that object to the netcdf file. This also adds the attributes
	// for the variables to the netcdf file
	vector<FONcBaseType *>::iterator i = _fonc_vars.begin() ;
	vector<FONcBaseType *>::iterator e = _fonc_vars.end() ;
	for( ; i != e; i++ )
	{
	    FONcBaseType *fbt = *i ;
	    fbt->define( _ncid ) ;
	}

	// Add any global attributes to the netcdf file
	AttrTable &globals = _dds->get_attr_table() ;
	BESDEBUG( "fonc", "Adding Global Attributes" << endl
			  << globals << endl ) ;
	FONcAttributes::addattrs( _ncid, NC_GLOBAL, globals, "", "" ) ;

	// We are done defining the variables, dimensions, and
	// attributes of the netcdf file. End the define mode.
	nc_enddef( _ncid ) ;

	// Write everything out
	i = _fonc_vars.begin() ;
	e = _fonc_vars.end() ;
	for( ; i != e; i++ )
	{
	    FONcBaseType *fbt = *i ;
	    fbt->write( _ncid ) ;
	}
    }
    catch( BESError &e )
    {
	nc_close( _ncid ) ;
	throw e ;
    }

    nc_close( _ncid ) ;
}

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including all of the FONc objects converted from DAP objects that are
 * to be sent to the netcdf file.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcTransform::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcTransform::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "ncid = " << _ncid << endl ;
    strm << BESIndent::LMarg << "temporary file = " << _localfile << endl ;
    BESIndent::Indent() ;
    vector<FONcBaseType *>::const_iterator i = _fonc_vars.begin() ;
    vector<FONcBaseType *>::const_iterator e = _fonc_vars.end() ;
    for( ; i != e; i++ )
    {
	FONcBaseType *fbt = *i ;
	fbt->dump( strm ) ;
    }
    BESIndent::UnIndent() ;
    BESIndent::UnIndent() ;
}

