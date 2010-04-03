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

#include <DDS.h>
#include <Structure.h>
#include <Array.h>
#include <Grid.h>
#include <Sequence.h>
#include <BESDebug.h>
#include <BESInternalError.h>

#define EMBEDDED_SEPARATOR "."
#define ATTRIBUTE_SEPARATOR "."
#define FONC_ORIGINAL_NAME "fonc_original_name"

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param localfile netcdf to create and write the information to
 * @throws BESInternalError if dds provided is empty or not read, if the
 * file is not specified or failed to create the netcdf file
 */
FONcTransform::FONcTransform( DDS *dds, BESDataHandlerInterface &dhi,
			      const string &localfile )
    : _ncid( 0 ), _dds( 0 ), _embedded_set( false ),
      _doing_grids( false ), _dim_name_num( 0 )
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
    // character then we will prefix it with _name_prefix. We will
    // get this prefix from the type of data that we are reading in,
    // such as nc, h4, h5, ff, jg, etc...
    dhi.first_container() ;
    if( dhi.container )
    {
	_name_prefix = dhi.container->get_container_type() + "_" ;
    }
    else
    {
	_name_prefix = "nc_" ;
    }

    int stax = nc_create( _localfile.c_str(), NC_CLOBBER, &_ncid ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"File out netcdf, "
	             + "unable to open file " + _localfile ;
	handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
FONcTransform::~FONcTransform()
{
    // delete the FONcGrid objects, but not the grid object itself or the
    // maps
    bool done = false ;
    while( !done )
    {
	vector<FONcGrid *>::iterator i = _grids.begin() ;
	vector<FONcGrid *>::iterator e = _grids.end() ;
	if( i == e )
	{
	    done = true ;
	}
	else
	{
	    FONcGrid *grid = (*i) ;
	    delete grid ;
	    _grids.erase( i ) ;
	}
    }

    done = false ;
    while( !done )
    {
	vector<FONcMap *>::iterator i = _maps.begin() ;
	vector<FONcMap *>::iterator e = _maps.end() ;
	if( i == e )
	{
	    done = true ;
	}
	else
	{
	    FONcMap *map = (*i) ;
	    delete map ;
	    _maps.erase( i ) ;
	}
    }
}

/** @brief Transforms each of the varaibles of the DataDDS to the netcdf file
 *
 * For each variable in the DataDDS write out that variable and its
 * attributes to the netcdf file. Each OPeNDAP data type translates into a
 * particular netcdf type. Also write out any global variables stored at the
 * top level of the DataDDS.
 */
int
FONcTransform::transform( )
{
    try
    {
	DDS::Vars_iter vi = _dds->var_begin() ;
	DDS::Vars_iter ve = _dds->var_end() ;
	for( ; vi != ve; vi++ )
	{
	    if( (*vi)->send_p() )
	    {
		BaseType *v = *vi ;
		//v->read("d1") ;
		switch( v->type() )
		{
		    case dods_str_c:
		    case dods_url_c:
			write_str( v ) ;
			break ;
		    case dods_byte_c:
		    case dods_int16_c:
		    case dods_uint16_c:
		    case dods_int32_c:
		    case dods_uint32_c:
		    case dods_float32_c:
		    case dods_float64_c:
			write_var( v ) ;
			break ;
		    case dods_grid_c:
			write_grid( v ) ;
			break ;
		    case dods_array_c:
			write_array( v ) ;
			break ;
		    case dods_structure_c:
			write_structure( v ) ;
			break ;
		    case dods_sequence_c:
			write_sequence( v ) ;
			break ;
		    default:
			string err = (string)"file out netcdf, unable to write "
				     + "unknown variable type" ;
			throw BESInternalError( err, __FILE__, __LINE__ ) ;

		}
	    }
	}
	// We've collected grids, if there were any. Go write them now
	write_grids() ;

	// These should be global attributes. Although, for multi-container
	// datasets, the globals are stored in the container structures.
	// They need to be tagged as such in some way.
	nc_redef( _ncid ) ;
	AttrTable &globals = _dds->get_attr_table() ;
	BESDEBUG( "fonc", "Adding Global Attributes" << endl
			  << globals << endl ) ;
	addattrs( NC_GLOBAL, globals, "", "" ) ;
	nc_enddef( _ncid ) ;
    }
    catch( BESError &e )
    {
	nc_close( _ncid ) ;
	throw e ;
    }

    nc_close( _ncid ) ;

    return 1 ;
}

/** @brief translate the OPeNDAP data type to a netcdf data type
 *
 * @param element The OPeNDAP element to translate
 * @return the netcdf data type
 */
nc_type
FONcTransform::get_nc_type( BaseType *element )
{
    nc_type x_type = NC_NAT ; // the constant ncdf uses to define simple type

    string var_type = element->type_name() ;
    if( var_type == "Byte" )        	// check this for dods type
	x_type = NC_BYTE ;
    else if( var_type == "String" )
	x_type = NC_CHAR ;
    else if( var_type == "Int16" )
	x_type = NC_SHORT ;
    else if( var_type == "UInt16" )
	x_type = NC_SHORT ;
    else if( var_type == "Int32" )
	x_type = NC_INT ;
    else if( var_type == "UInt32" )
	x_type = NC_INT ;
    else if( var_type == "Float32" )
	x_type = NC_FLOAT ;
    else if( var_type == "Float64" )
	x_type = NC_DOUBLE ;

    return x_type ;
}

/** @brief transorm an OPeNDAP structure to netcdf.
 *
 * Structures do not have a representation in netcdf. What we do here is
 * flatten the structure. For each variable in the structure we append the
 * name of the structure to create a new variable. For example, if we have a
 * structure Point with members x and y we end up creating two variables in
 * netcdf Point.x and Point.y.
 *
 * @param b The Structure to convert
 * @throws BESInternalError if problem writing out the structure to netcdf
 */
void
FONcTransform::write_structure( BaseType* b )
{
    Structure *s = dynamic_cast<Structure *>(b) ;
    if( !s )
    {
	string s = (string)"File out netcdf, write_structure passed a variable "
	           + "that is not a structure" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
    string myname = s->name() ;
    BESDEBUG( "fonc", "FONcTransform::write_structure for "
                      << myname << endl ) ;

    // add this structure to the embedded list for name generation and
    // attributes to add.
    add_embedded( b ) ;

    try
    {
	Constructor::Vars_iter vi = s->var_begin() ;
	Constructor::Vars_iter ve = s->var_end() ;
	for( ; vi != ve; vi++ )
	{
	    BaseType *bt = *vi ;
	    if( bt->send_p() )
	    {
		BESDEBUG( "fonc", "FONcTransform::write_structure, done "
				  << "writing " << bt->name() << endl ) ;
		switch( bt->type() )
		{
		    case dods_str_c:
		    case dods_url_c:
			write_str( bt ) ;
			break ;
		    case dods_byte_c:
		    case dods_int16_c:
		    case dods_uint16_c:
		    case dods_int32_c:
		    case dods_uint32_c:
		    case dods_float32_c:
		    case dods_float64_c:
			write_var( bt ) ;
			break ;
		    case dods_grid_c:
			write_grid( bt ) ;
			break ;
		    case dods_array_c:
			write_array( bt ) ;
			break ;
		    case dods_structure_c:
			write_structure( bt ) ;
			break ;
		    case dods_sequence_c:
			write_sequence( bt ) ;
			break ;
		    default:
		    {
			string s = (string)"File out netcdf, "
				   + "write_structure for unknown type "
				   + bt->type_name() ;
			throw BESInternalError( s, __FILE__, __LINE__ ) ;
		    }
		    break ;
		}
		BESDEBUG( "fonc", "FONcTransform::write_structure, done "
				  << "writing " << bt->name() << endl ) ;
	    }
	}
    }
    catch( BESError &e )
    {
	remove_embedded( b ) ;
	throw e ;
    }

    // remove this structure from the list of embedded elements
    remove_embedded( b ) ;

    BESDEBUG( "fonc", "FONcTransform::write_structure done for "
                      << myname << endl ) ;
}

/** @brief write out an OPeNDAP Grid to netcdf
 *
 * Grids do not have a representation in netcdf. Like structures, we flatten
 * out the grid, but in a different way. First we save off all of the grids.
 * Once we have saved all of the grids we go through them to see if there
 * are any shared maps. If there are then that map is only written once (if
 * not already written in the netcdf file) and referenced by each of the
 * grid arrays eventually written to represent the Grid.
 *
 * @param b The Grid to write to netcdf
 * @throws BESInternalError if there is a problem writing the grid maps or
 * grid array to netcdf
 */
void
FONcTransform::write_grid( BaseType* b )
{
    // Grids will be done all at once. What we want to do right here is
    // simply save the grid to a list. Once all other processing has been
    // done for all other variables then we'll go through the grids, check
    // for coordinate variables, etc...
    Grid *g = dynamic_cast<Grid *>(b) ;
    if( !g )
    {
	string s = (string)"File out netcdf, write_grid passed a variable "
	           + "that is not a grid" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
    FONcGrid *grid = new FONcGrid( embedded_name( "" ), g ) ;
    _grids.push_back( grid ) ;
}

/** @brief Function that actually writes out the grids
 *
 * @see write_grid
 * @throws BESInternalError if there is a problem writing out the grids
 */
void
FONcTransform::write_grids()
{
    BESDEBUG( "fonc", "FONcTransform::write_grids - writing any grids"
		      << endl ) ;
    _doing_grids = true ;
    // need to compare all of the grids and try to find common maps with
    // common dimensions, types, and values. If they match, then they
    // are coordinate variables and need to be written to the root
    // namespace along with their dimensions.
    vector<FONcGrid *>::iterator gi = _grids.begin() ;
    vector<FONcGrid *>::iterator ge = _grids.end() ;
    for( ; gi != ge; gi++ )
    {
	Grid *grid = (*gi)->grid ;
	string full_name ;
	if( (*gi)->embedded_name.empty() )
	{
	    full_name = grid->name() ;
	}
	else
	{
	    full_name = (*gi)->embedded_name
			+ EMBEDDED_SEPARATOR
			+ grid->name() ;
	}
	Grid::Map_iter mi = grid->map_begin() ;
	Grid::Map_iter me = grid->map_end() ;
	for( ; mi != me; mi++ )
	{
	    Array *map = dynamic_cast<Array *>( (*mi) ) ;
	    if( !map )
	    {
		string err = (string)"file out netcdf, grid "
			     + grid->name() + " map is not an array" ;
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	    }

	    vector<FONcMap *>::iterator vi = _maps.begin() ;
	    vector<FONcMap *>::iterator ve = _maps.end() ;
	    FONcMap *map_found = 0 ;
	    bool done = false ;
	    for( ; vi != ve && !done; vi++ )
	    {
		map_found = (*vi) ;
		if (!map_found) {
			throw BESInternalError("map_found is null.", __FILE__, __LINE__);
		}
		done = map_found->compare( map ) ;
	    }
	    // if we didn't find a match then done is still false. Add the
	    // map to the vector of maps. If they are the same then create a
	    // new FONcMap, add the grid name to the shared list and add the
	    // FONcMap to the FONcGrid.
	    if( !done )
	    {
		map_found = new FONcMap( full_name, (*gi)->embedded_name, map );
		_maps.push_back( map_found ) ;
	    }
	    else
	    {
		// it's the same ... we are sharing. Add the grid name fo
		// the list of grids sharing this map and set the embedded
		// name to empty, just using the name of the map.
		map_found->shared_by_grids.push_back( full_name ) ;
		map_found->embedded_name = "" ;
	    }
	    (*gi)->maps.push_back( map_found ) ;
	}
    }

    BESDEBUG( "fonc", *this << endl ) ;

    // now, we should have everything that we need to write out all of the
    // arrays. when we write out the arrays we record the dimension ids
    // created for the dimensions to use in writing out the grid array.

    // Iterate through the _maps and write out all of those arrays keeping
    // track of the dimension ids. We might have string maps, which will
    // have two dimensions. We only care about the first dimension, though.
    int dimids[2] ;
    dimids[0] = 0 ;
    dimids[1] = 0 ;

    vector<FONcMap *>::iterator mi = _maps.begin() ;
    vector<FONcMap *>::iterator me = _maps.end() ;
    for( ; mi != me; mi++ )
    {
	if( (*mi)->written == false )
	{
	    // FIXME: what about embedded if not shared
	    bool is_single = false ;
	    if( (*mi)->shared_by_grids.size() == 1 )
	    {
		is_single = true ;
		set_embedded( (*mi)->embedded_name ) ;
	    }
	    Array *a = (*mi)->map ;
	    BESDEBUG( "fonc", "write_grids writing map "
			      << a->name() << endl ) ;
	    write_array( (*mi)->map, dimids ) ;
	    (*mi)->dimid = dimids[0] ;
	    if( is_single )
	    {
		unset_embedded() ;
	    }
	}
    }

    BESDEBUG( "fonc", *this << endl ) ;

    // write out the grid arrays now given the dimensions
    gi = _grids.begin() ;
    ge = _grids.end() ;
    for( ; gi != ge; gi++ )
    {
	Array *a = (*gi)->grid->get_array() ;
	BESDEBUG( "fonc", "write_grids writing grid " << a->name() << endl ) ;
	nc_type array_type = get_nc_type( a->var() ) ;
	int ndims = (*gi)->maps.size() ;
	BESDEBUG( "fonc", "    ndims = " << ndims << endl ) ;
	int *gdims = new int[ndims];
	int *gdim_sizes = new int[ndims];
#if 0
	int gdims[ndims] ;
	int gdim_sizes[ndims] ;
#endif
	int nelements = 1 ;
	vector<FONcMap *>::iterator gmi = (*gi)->maps.begin() ;
	vector<FONcMap *>::iterator gme = (*gi)->maps.end() ;
	int dimnum = 0 ;
	for( ; gmi != gme; gmi++ )
	{
	    if (!(dimnum < ndims)) {
	        delete[] gdims;
	        delete[] gdim_sizes;
	        throw BESInternalError("dimnum too big", __FILE__, __LINE__);
	    }

	    gdims[dimnum] = (*gmi)->dimid ;
	    gdim_sizes[dimnum] = (*gmi)->map->length() ;
	    nelements *= gdim_sizes[dimnum] ;
	    dimnum++ ;
	}
	BESDEBUG( "fonc", "    nelements = " << nelements << endl ) ;

	// need to do this for each grid array because the write_array
	// method ends the definition
	nc_redef( _ncid ) ;

	set_embedded( (*gi)->embedded_name ) ;
	int varid =
	    write_array( a, array_type, nelements, ndims, gdims, gdim_sizes ) ;
	unset_embedded() ;
	BESDEBUG( "fonc", "write_grids done writing grid "
			  << a->name() << endl ) ;

	// add the attributes for this grid. Definition mode was turned off
	// in the write_array call.
	BESDEBUG( "fonc", "write_grids writing grid attrs for "
			  << a->name() << endl ) ;
	nc_redef( _ncid ) ;
	add_attributes( varid, (*gi)->grid ) ;
	nc_enddef( _ncid ) ;
	BESDEBUG( "fonc", "write_grids done writing grid attrs for "
			  << a->name() << endl ) ;

	delete[] gdims;
        delete[] gdim_sizes;
    }

    _doing_grids = false ;
    BESDEBUG( "fonc", "FONcTransform::write_grids - done writing grids"
		      << endl) ;
}

/** @brief a method to compare two grid maps, or possible grid maps.
 *
 * All arrays are saved as an FONcMap if the array has only one dimension
 * and the name of the array and the name of the dimension are the same. The
 * maps are the same if their names are the same, they have the same number
 * of dimensions (arrays of strings written out have 2 dimensions, one for
 * the max length of the string), the type of the maps are the same, the
 * dimension size is the same, the dimension names are the same, and the
 * values of the maps are the same.
 *
 * @param tomap compare the saved map to this provided map
 * @return true if they are the same (shared) or false otherwise
 */
bool
FONcTransform::FONcMap::compare( Array *tomap )
{
    BESDEBUG( "fonc",
	      "comparing " << tomap->name()
	      << " to " << map->name() << endl ) ;
    // compare the name
    if( tomap->name() != map->name() ) return false ;
    // compare the type
    if( tomap->var()->type() != map->var()->type() ) return false ;
    // compare the length of the array
    if( tomap->length() != map->length() ) return false ;
    // compare the number of dimensions
    if( tomap->dimensions() != map->dimensions() ) return false ;
    // the variable name needs to be the same as the dimension name
    if( map->dimension_name( map->dim_begin() ) != map->name() ) return false ;
    // compare the dimension name
    if( tomap->dimension_name( tomap->dim_begin() ) !=
	map->dimension_name( map->dim_begin() ) ) return false ;
    // compare the dimension size. Is this the same as the length of the array
    if( tomap->dimension_size( tomap->dim_begin(), true ) !=
	map->dimension_size( map->dim_begin(), true ) ) return false ;
    // compare the values of the array
    switch( tomap->var()->type() )
    {
	case dods_byte_c:
	    {
		dods_byte my_values[map->length()] ;
		map->value( my_values ) ;
		dods_byte to_values[map->length()] ;
		tomap->value( to_values ) ;
		for( int i = 0; i < map->length(); i++ )
		{
		    if( my_values[i] != to_values[i] ) return false ;
		}
	    }
	    break ;
	case dods_int16_c:
	    {
		dods_int16 my_values[map->length()] ;
		map->value( my_values ) ;
		dods_int16 to_values[map->length()] ;
		tomap->value( to_values ) ;
		for( int i = 0; i < map->length(); i++ )
		{
		    if( my_values[i] != to_values[i] ) return false ;
		}
	    }
	    break ;
	case dods_uint16_c:
	    {
		dods_uint16 my_values[map->length()] ;
		map->value( my_values ) ;
		dods_uint16 to_values[map->length()] ;
		tomap->value( to_values ) ;
		for( int i = 0; i < map->length(); i++ )
		{
		    if( my_values[i] != to_values[i] ) return false ;
		}
	    }
	    break ;
	case dods_int32_c:
	    {
		dods_int32 my_values[map->length()] ;
		map->value( my_values ) ;
		dods_int32 to_values[map->length()] ;
		tomap->value( to_values ) ;
		for( int i = 0; i < map->length(); i++ )
		{
		    if( my_values[i] != to_values[i] ) return false ;
		}
	    }
	    break ;
	case dods_uint32_c:
	    {
		dods_uint32 my_values[map->length()] ;
		map->value( my_values ) ;
		dods_uint32 to_values[map->length()] ;
		tomap->value( to_values ) ;
		for( int i = 0; i < map->length(); i++ )
		{
		    if( my_values[i] != to_values[i] ) return false ;
		}
	    }
	    break ;
	case dods_float32_c:
	    {
		dods_float32 my_values[map->length()] ;
		map->value( my_values ) ;
		dods_float32 to_values[map->length()] ;
		tomap->value( to_values ) ;
		for( int i = 0; i < map->length(); i++ )
		{
		    if( my_values[i] != to_values[i] ) return false ;
		}
	    }
	    break ;
	case dods_float64_c:
	    {
		dods_float64 my_values[map->length()] ;
		map->value( my_values ) ;
		dods_float64 to_values[map->length()] ;
		tomap->value( to_values ) ;
		for( int i = 0; i < map->length(); i++ )
		{
		    if( my_values[i] != to_values[i] ) return false ;
		}
	    }
	    break ;
	case dods_str_c:
	case dods_url_c:
	    {
		vector<string> my_values ;
		map->value( my_values ) ;
		vector<string> to_values ;
		tomap->value( to_values ) ;
		vector<string>::const_iterator mi = my_values.begin() ;
		vector<string>::const_iterator me = my_values.end() ;
		vector<string>::const_iterator ti = to_values.begin() ;
		for( ; mi != me; mi++, ti++ )
		{
		    if( (*mi) != (*ti) ) return false ;
		}
	    }
	    break ;
    }

    BESDEBUG( "fonc", "same" << endl ) ;
    return true ;
}

FONcTransform::FONcDimSet::FONcDimSet( int ndims )
    : numdims( ndims )
{
}

void
FONcTransform::FONcDimSet::add_dimension( Array *a, Array::Dim_iter di )
{
    dimnames.push_back( a->dimension_name( di ) ) ;
    dimsizes.push_back( a->dimension_size( di, true ) ) ;
}

bool
FONcTransform::FONcDimSet::check_dims( FONcDimSet *set, int dims[],
				       int dim_sizes[], int ndims,
				       int &nelements )
{
    if( set->numdims != numdims )
    {
	return false ;
    }

    int index = 0 ;
    for( index = 0; index < numdims; index++ )
    {
	if( set->dimnames[index] != dimnames[index] )
	{
	    return false ;
	}
	if( set->dimsizes[index] != dimsizes[index] )
	{
	    return false ;
	}
    }

    if( ndims < numdims )
    {
	string s = (string)"FONcDimSet::check_dims not enough space in "
	           + "dims and dimsizes" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }

    for( index = 0; index < numdims; index++ )
    {
	dims[index] = dimnums[index] ;
	dim_sizes[index] = dimsizes[index] ;
	nelements *= dimsizes[index] ;
    }

    return true ;
}

int
FONcTransform::FONcDimSet::add_dims( int ncid, int dims[],
				     int dim_sizes[], int ndims,
				     int &nelements,
				     unsigned int &dim_name_num,
				     const string &name_prefix )
{
    if( ndims < numdims )
    {
	string s = (string)"FONcDimSet::add_dims - not enough space in "
		   "dims and dimsizes" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
    for( int index = 0; index < numdims; index++ )
    {
	if( dimnames[index].empty() )
	{
	    ostringstream dimname_strm ;
	    dimname_strm << "dim" << dim_name_num+1 ;
	    dim_name_num++ ;
	    ncdimnames.push_back( dimname_strm.str() ) ;
	}
	else
	{
	    ncdimnames.push_back( dimnames[index] ) ;
	}
	ncdimnames[index] = FONcUtils::id2netcdf( ncdimnames[index],
						  name_prefix ) ;
	int this_dimension = 0 ;
	int stax = nc_def_dim( ncid, ncdimnames[index].c_str(),
			       dimsizes[index], &this_dimension ) ;
	if( stax != NC_NOERR )
	{
	    return stax ;
	}
	dimnums.push_back( this_dimension ) ;
	dims[index] = this_dimension ;
	dim_sizes[index] = dimsizes[index] ;
	nelements *= dimsizes[index] ;
    }
    return NC_NOERR ;
}

/** @brief write an OPeNDAP Array to netcdf
 *
 * Writes out an OpeNDAP Array instance out to the netcdf file. First the
 * dimensions are defined in the netcdf file and then the array values are
 * written. Arrays of Str values are written by adding a new dimension to
 * represent the maximum length of the strings
 *
 * @param b OPeNDAP Array to write to netcdf
 * @param dimids out parameter to store the dimension ids of the dimensions
 * defined. If NULL is passed then no dimension ids are returned
 * @throws BESInternalError if there is a problem writing the Array to netcdf
 */
void
FONcTransform::write_array( BaseType* b, int dimids[] )
{
    Array *a = dynamic_cast<Array *>( b ) ;
    if( !a )
    {
	string s = (string)"File out netcdf, write_array passed a variable "
	           + "that is not an array" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }

    string varname = embedded_name( a->name() ) ;
    BESDEBUG( "fonc", "FONcTransform::write_array for array "
                      << varname << endl ) ;

    nc_type array_type = get_nc_type( a->var() ) ;
    int ndims = a->dimensions() ;
    int actual_ndims = ndims ;
    if( array_type == NC_CHAR )
    {
	// if we have array of strings then we need to add the string length
	// dimension, so add one more to ndims
	ndims++ ;
    }

    nc_redef( _ncid ) ;

    int *dims = new int[ndims];
    int *dim_sizes = new int[ndims];
    int nelements = 1 ;

    // Add each dimension to a dimension set. this keeps the order of
    // the dimensions, the size of the dimensions, and the names of the
    // dimensions. A shared dimension must have the same name, size, and
    // belong to arrays of the same dimensionality.
    FONcDimSet *set = new FONcDimSet( actual_ndims ) ;
    Array::Dim_iter di = a->dim_begin() ;
    Array::Dim_iter de = a->dim_end() ;
    for( ; di != de; di++ )
    {
	set->add_dimension( a, di ) ;
    }

    // Take the new dimension set and compare it to the already exiting
    // dimension sets to make see if there are any shared dimensions. A
    // shared dimension must have the same name, size, and belong to
    // arrays of the same dimensionality
    vector<FONcDimSet *>::iterator dsi = _dims.begin() ;
    vector<FONcDimSet *>::iterator dse = _dims.end() ;
    bool found = false ;
    for( ; dsi != dse && !found; dsi++ )
    {
	found = (*dsi)->check_dims( set, dims, dim_sizes,
				    actual_ndims, nelements ) ;
    }

    if( !found )
    {
	// If these aren't share dimensions, haven't been defined before in
	// other words, then add the dimensions to the netcdf target file,
	// getting their dimension ids for defining the array.
	int stax = set->add_dims( _ncid, dims, dim_sizes,
				  actual_ndims, nelements,
				  _dim_name_num,
				  _name_prefix ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"fileout.netcdf - "
			 + "Failed to add dimensions" ;
	    handle_error( stax, err, __FILE__, __LINE__ ) ;
	}
	_dims.push_back( set ) ;
    }
    else
    {
	// If the are shared already, then delete this new set and use
	// the ones that matched.
	delete set ;
	set = 0 ;
    }

    if( dimids )
    {
	for( int index = 0; index < actual_ndims; index++ )
	{
	    dimids[index] = dims[index] ;
	}
    }

    // Now create the array
    write_array( a, array_type, nelements, ndims, dims, dim_sizes ) ;

    delete[] dims;
    delete[] dim_sizes;
}

/** @brief helper function to write an array.
 *
 * This is provided so that writing grids can use this part of the array
 * write without using the one above, as the dimensions are already written.
 *
 * @param a OPeNDAP Array to write to netcdf
 * @param array_type netcdf type of data stored in the Array
 * @param nelements Number of elements in the array
 * @param ndims number of dimensions in the Array
 * @param dims netcdf dimension ids of each of the Array dimensions
 * @param dim_sizes the size of each of the Array's dimensions
 * @return netcdf variable id of the new variable
 * @throws BESInternalError if there is a problem writing the Array
 */
int
FONcTransform::write_array( Array *a, nc_type array_type,
			    int nelements,
			    int ndims, int dims[], int dim_sizes[] )
{
    ncopts = NC_VERBOSE ;
    int stax = NC_NOERR ;
    string tmp_varname = embedded_name( a->name() ) ;
    string varname = FONcUtils::id2netcdf( tmp_varname,
					   _name_prefix ) ;

    // if the variable name has been adjusted to be netcdf compliant
    // then add an attribute "original_name" with the original name
    // of the variable
    if( tmp_varname != varname )
    {
	add_original_attr( a, tmp_varname ) ;
    }

    int varid = 0 ;
    if( array_type != NC_CHAR )
    {
	stax = nc_def_var( _ncid, varname.c_str(), array_type,
			   ndims, dims, &varid ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"fileout.netcdf - "
			 + "Failed to define variable "
			 + varname ;
	    handle_error( stax, err, __FILE__, __LINE__ ) ;
	}
	add_attributes( varid, a ) ;
	nc_enddef( _ncid ) ;

	if( !_doing_grids )
	{
	    if( a->name() == a->dimension_name( a->dim_begin() )
		&& a->dimensions() == 1 )
	    {
		_maps.push_back( new FONcMap( a, dims[0] ) ) ;
	    }
	}

	// create array to hold data hyperslab
	switch( array_type )
	{
	    case NC_BYTE:
		{
		    unsigned char *data = new unsigned char[nelements] ;
		    a->buf2val( (void**)&data ) ;
		    stax = nc_put_var_uchar( _ncid, varid, data ) ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of bytes for "
				     + varname ;
			handle_error( stax, err, __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    case NC_SHORT:
		{
		    short *data = new short [nelements] ;
		    a->buf2val( (void**)&data ) ;
		    int stax = nc_put_var_short( _ncid, varid, data ) ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of shorts for "
				     + varname ;
			handle_error( stax, err, __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    case NC_INT:
		{
		    int *data = new int[nelements] ;
		    a->buf2val( (void**)&data ) ;
		    int stax = nc_put_var_int( _ncid, varid, data ) ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of ints for "
				     + varname ;
			handle_error( stax, err, __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    case NC_FLOAT:
		{
		    float *data = new float[nelements] ;
		    a->buf2val( (void**)&data ) ;
		    int stax = nc_put_var_float( _ncid, varid, data ) ;
		    ncopts = NC_VERBOSE ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of floats for "
				     + varname ;
			handle_error( stax, err, __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    case NC_DOUBLE:
		{
		    double *data = new double[nelements] ;
		    a->buf2val( (void**)&data ) ;
		    int stax = nc_put_var_double( _ncid, varid, data ) ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of doubles for "
				     + varname ;
			handle_error( stax, err, __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    default:
		string err = "Failed to transform unknown type in file out netcdf" ;
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	} ;
    }
    else
    {
	// special case for string data. We need to add another dimension
	// for the length of the string.

	// get the data from the dap array
	int array_length = a->length() ;
	string *data = new string[array_length] ;
	a->buf2val( (void**)&data ) ;

	// determine the max length of the strings
	int max_length = 0 ;
	for( int i = 0; i < array_length; i++ )
	{
	    if( data[i].length() > max_length )
	    {
		max_length = data[i].length() ;
	    }
	}
	max_length++ ;
	dim_sizes[ndims-1] = max_length ;

	// create the string dimension with the max length
	string lendim_name = varname + "_len" ;
	int this_dimension = 0 ;
	stax = nc_def_dim( _ncid, lendim_name.c_str(),
			   max_length, &this_dimension ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"fileout.netcdf - "
			 + "Failed to define dimension "
			 + lendim_name ;
	    handle_error( stax, err, __FILE__, __LINE__ ) ;
	}
	dims[ndims-1] = this_dimension ;

	stax = nc_def_var( _ncid, varname.c_str(), array_type,
			   ndims, dims, &varid ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"fileout.netcdf - "
			 + "Failed to define variable "
			 + varname ;
	    handle_error( stax, err, __FILE__, __LINE__ ) ;
	}
	add_attributes( varid, a ) ;

	nc_enddef( _ncid ) ;

	if( !_doing_grids )
	{
	    if( a->name() == a->dimension_name( a->dim_begin() )
		&& a->dimensions() == 2 )
	    {
		_maps.push_back( new FONcMap( a, dims[0] ) ) ;
	    }
	}

	size_t var_count[ndims] ;
	size_t var_start[ndims] ;
	int dim = 0 ;
	for( dim = 0; dim < ndims; dim++ )
	{
	    // the count for each of the dimensions will always be 1 except
	    // for the string length dimension
	    var_count[dim] = 1 ;

	    // the start for each of the dimensions will start at 0. We will
	    // bump this up in the while loop below
	    var_start[dim] = 0 ;
	}

	for( int element = 0; element < nelements; element++ )
	{
#if 0
	    const char *val = data[element].c_str() ;
#endif
#if 0
	    var_count[ndims-1] = strlen( val ) + 1 ;
#endif
	    var_count[ndims-1] = data[element].size() + 1 ;
	    var_start[ndims-1] = 0 ;

	    // write out the string
#if 0
	    stax = nc_put_vara_text( _ncid, varid, var_start, var_count, val ) ;
#endif
	    stax = nc_put_vara_text( _ncid, varid, var_start, var_count, 
				     data[element].c_str() ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"fileout.netcdf - "
			     + "Failed to create array of strings for "
			     + varname ;
		handle_error( stax, err, __FILE__, __LINE__ ) ;
	    }

	    // bump up the start.
	    if( element+1 < nelements )
	    {
		bool done = false ;
		dim = ndims-2 ;
		while( !done )
		{
		    var_start[dim] = var_start[dim] + 1 ;
		    if( var_start[dim] == dim_sizes[dim] )
		    {
			var_start[dim] = 0 ;
			dim-- ;
		    }
		    else
		    {
			done = true ;
		    }
		}
	    }
	}

	delete [] data ;
    }

    BESDEBUG( "fonc", "FONcTransform::write_array done for "
                      << varname << endl ) ;
    return varid ;
}

/** @brief Write a simple string variable to netcdf
 *
 * An OPeNDAP Str is converted into an array of characters where the
 * dimension is the maximum length of the strings being written
 *
 * @param b The OPenDAP Str to write to netcdf
 * @throws BESInternalError if problem writing the Str to netcdf
 */
void
FONcTransform::write_str( BaseType *b )
{
    string tmp_varname = embedded_name( b->name() ) ;
    string varname = FONcUtils::id2netcdf( tmp_varname,
					   _name_prefix ) ;

    // if the variable name has been adjusted to be netcdf compliant
    // then add an attribute "original_name" with the original name
    // of the variable
    if( tmp_varname != varname )
    {
	add_original_attr( b, tmp_varname ) ;
    }

    BESDEBUG( "fonc", "FONcTransform::write_str for var "
                      << varname << endl ) ;
    int chid ;			// dimension id for char positions
    int varid ;			// netCDF variable id
    int var_dims[1] ;		// variable shape
    size_t var_start[1] ;	// variable start
    size_t var_count[1] ;	// variable count

    nc_type var_type = get_nc_type( b ) ;	// translate dap type to nc
    if( var_type != NC_CHAR )
    {
	string err = (string)"file out netcdf - write_str called for "
	             + "non string type " + b->type_name() ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    nc_redef( _ncid ) ;				// go into def mode

    string *data = new string ;
    b->buf2val( (void**)&data ) ;
#if 0
    const char *val = data->c_str() ;
#endif

    string dimname = varname + "_len" ;
#if 0
    int stax = nc_def_dim( _ncid, dimname.c_str(), strlen( val )+1, &chid ) ;
#endif
    int stax = nc_def_dim( _ncid, dimname.c_str(), data->size() + 1, &chid ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to define dim " + dimname + " for "
		     + varname ;
	handle_error( stax, err, __FILE__, __LINE__ ) ;
    }

    var_dims[0] = chid ;
    stax = nc_def_var( _ncid, varname.c_str(), NC_CHAR, 1, var_dims, &varid );
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to define var " + varname ;
	handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    add_attributes( varid, b ) ;

    nc_enddef( _ncid ) ;

#if 0
    var_count[0] = strlen( val) + 1 ;
#endif
    var_count[0] = data->size() + 1 ;
    var_start[0] = 0 ;
#if 0
    stax = nc_put_vara_text( _ncid, varid, var_start, var_count, val ) ;
#endif
    stax = nc_put_vara_text( _ncid, varid, var_start, var_count, 
			     data->c_str() ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to write string data " + *data + " for "
		     + varname ;
	delete data ;
	handle_error( stax, err, __FILE__, __LINE__ ) ;
    }

    delete data ;

    BESDEBUG( "fonc", "FONcTransform::write_str done for "
                      << varname << endl ) ;
}

/** @brief Write a simple type variable to netcdf
 *
 * This method writes out simple varialbes of type byte, short, int, float,
 * double.
 *
 * @param b The OPenDAP data to write to netcdf
 * @throws BESInternalError if there is a problem writing out the variable
 * to netcdf
 */
void
FONcTransform::write_var( BaseType* b )
{
    string tmp_varname = embedded_name( b->name() ) ;
    string varname = FONcUtils::id2netcdf( tmp_varname,
					   _name_prefix ) ;

    // if the variable name has been adjusted to be netcdf compliant
    // then add an attribute "original_name" with the original name
    // of the variable
    if( tmp_varname != varname )
    {
	add_original_attr( b, tmp_varname ) ;
    }

    BESDEBUG( "fonc", "FONcTransform::write_var for var "
                      << varname << endl ) ;
    int varid ;
    size_t var_index[] = {0} ;
    nc_type var_type = get_nc_type( b ) ;

    nc_redef( _ncid ) ;

    int stax = nc_def_var( _ncid, varname.c_str(), var_type, 0, NULL, &varid ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to define variable "
		     + varname ;
	handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    add_attributes( varid, b ) ;

    nc_enddef( _ncid ) ;

    switch( var_type )
    {
	case NC_BYTE:
	    {
		unsigned char *data = new unsigned char ;
		b->buf2val( (void**)&data ) ;
		stax = nc_put_var1_uchar( _ncid, varid, var_index, data ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write byte data for "
				 + varname ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_SHORT:
	    {
		short *data = new short ;
		b->buf2val( (void**)&data ) ;
		stax = nc_put_var1_short( _ncid, varid, var_index, data ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write short data for "
				 + varname ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_INT:
	    {
		int *data = new int ;
		b->buf2val( (void**)&data ) ;
		stax = nc_put_var1_int( _ncid, varid, var_index, data ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write int data for "
				 + varname ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_FLOAT:
	    {
		float *data = new float ;
		b->buf2val( (void**)&data ) ;
		stax = nc_put_var1_float( _ncid, varid, var_index, data ) ;
		ncopts = NC_VERBOSE ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write float data for "
				 + varname ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_DOUBLE:
	    {
		double *data = new double ;
		b->buf2val( (void**)&data ) ;
		stax = nc_put_var1_double( _ncid, varid, var_index, data ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write double data for "
				 + varname ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	default:
	    {
		string err = (string)"fileout.netcdf - "
			     + "Unable to write netcdf data type "
			     + b->type_name() + " for " + varname ;
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	    }
    }
    BESDEBUG( "fonc", "FONcTransform::write_var done for "
                      << varname << endl ) ;
}

/** @brief Add the attributes for an OPeNDAP variable to the netcdf file
 *
 * This method writes out any attributes for the probided variable and for
 * any parent BaseType objects of this variable. For example, if this
 * variable is a part of a structure, which is a part of another structure,
 * then we write out all of the attributes for the outermost structure, the
 * inner structure, and the variable of the structure. For example, if there
 * is a structure Alpha with attribyte a1, that contains a structure Point
 * with attribyte s2, which containts two variables x, with attribute a3,
 * and y, with attribute a4, then variable Alpha.Point.x will contain
 * attribytes Alpha.a1, Alpha.Point.s2, and a3 and Alpha.Point.y will
 * contain attributes Alpha.a1, Alpha.Point.s2, and a4.
 *
 * Attributes can also be attribute containers, and not just simple
 * attributes. In this case, just as with structures, we flatten out the
 * attribute containers. For example, if there is an attribute container
 * called name, with attributes first and last contained in it, then two
 * attributes are created called name.first and name.last.
 *
 * OPeNDAP can have multiple string values for a given attribute. This can
 * not be represented in netcdf3 (can in netcdf4). To accomplish this we
 * take each of the string values for the given attribute and append them
 * together using a newline as a separator (recommended by Unidata).
 *
 * @param varid The netcdf variable id to associate the attributes to
 * @param b The OPeNDAP variable containing the attributes.
 * @throws BESInternalError if there is a problem writing the attributes for
 * the variable.
 */
void
FONcTransform::add_attributes( int varid, BaseType *b )
{
    // add the attributes from b to this varid and all the attributes from
    // any embedded structures/grids.
    vector<BaseType *>::const_iterator i = _embedded.begin() ;
    vector<BaseType *>::const_iterator e = _embedded.end() ;
    string new_name ;
    for( ; i != e; i++ )
    {
	if( !new_name.empty() )
	{
	    new_name += EMBEDDED_SEPARATOR ;
	}
	new_name += (*i)->name() ;
	addattrs( varid, (*i), new_name ) ;
    }
    addattrs( varid, b, "" ) ;
}

/** @brief helper function for add_attributes
 *
 * @param varid The netcdf variable id
 * @param b The OPenDAP data object that contains the attribytes
 * @param var_name any variable name to prepend to the attribute name
 * @throws BESInternalError if there are any problems writing out the
 * attribytes for the data object.
 */
void
FONcTransform::addattrs( int varid, BaseType *b, const string &var_name )
{
    AttrTable &attrs = b->get_attr_table() ;
    addattrs( varid, attrs, var_name, "" ) ;
}

/** @brief helper function for add_attributes
 *
 * @param varid The netcdf variable id
 * @param attrs The OPenDAP AttrTable containing the attributes
 * @param var_name any variable name to prepend to the attribute name
 * @param prepend_attr Any name to prepend to the name of the attribute
 * @throws BESInternalError if there are any problems writing out the
 * attribytes for the data object.
 */
void
FONcTransform::addattrs( int varid, AttrTable &attrs,
			 const string &var_name, const string &prepend_attr )
{
    unsigned int num_attrs = attrs.get_size() ;
    if( num_attrs )
    {
	AttrTable::Attr_iter i = attrs.attr_begin() ;
	AttrTable::Attr_iter e = attrs.attr_end() ;
	for( ; i != e; i++ )
	{
	    unsigned int num_vals = attrs.get_attr_num( i ) ;
	    if( num_vals )
	    {
		addattrs( varid, var_name, attrs, i, prepend_attr ) ;
	    }
	}
    }
}

/** @brief helper function for add_attributes that writes out a single
 * attribute
 *
 * @param varid The netcdf variable id
 * @param var_name the name of the variable (flattened)
 * @param attrs the AttrTable that contains the attribute to be written
 * @param attr the iterator into the AttrTable for the attribute to be written
 * @param prepend_attr any attribute name to prepend to the name of this
 * attribute
 * @throws BESInternalError if there is a problem writing this attribute
 */
void
FONcTransform::addattrs( int varid, const string &var_name,
			 AttrTable &attrs,
			 AttrTable::Attr_iter &attr,
			 const string &prepend_attr )
{
    string attr_name = attrs.get_name( attr ) ;
    string new_attr_name ;
    if( !prepend_attr.empty() )
    {
	new_attr_name = prepend_attr + EMBEDDED_SEPARATOR + attr_name ;
    }
    else
    {
	new_attr_name = attr_name ;
    }

    string new_name ;
    if( !var_name.empty() )
    {
	new_name = var_name + ATTRIBUTE_SEPARATOR + new_attr_name ;
    }
    else
    {
	new_name = new_attr_name ;
    }
    new_name = FONcUtils::id2netcdf( new_name,
				     _name_prefix ) ;
    if( varid == NC_GLOBAL )
	BESDEBUG( "fonc", "Adding global attributes " << new_name << endl ) ;
    else
	BESDEBUG( "fonc", "Adding attributes " << new_name << endl ) ;

    int stax = NC_NOERR ;
    unsigned int attri = 0 ;
    AttrType type = attrs.get_attr_type( attr ) ;
    unsigned int num_vals = attrs.get_attr_num( attr ) ;
    switch( type )
    {
	case Attr_container:
	    {
		// flatten
		BESDEBUG( "fonc", "Attribute " << new_name
				  << " is an attribute container" << endl ) ;
		AttrTable *container = attrs.get_attr_table( attr ) ;
		if( container )
		{
		    addattrs( varid, *container, var_name, new_attr_name ) ;
		}
	    }
	    break ;
	case Attr_byte:
	    {
		// unsigned char
		unsigned char vals[num_vals] ;
		for( attri = 0; attri < num_vals; attri++ )
		{
		    string val = attrs.get_attr( attr, attri ) ;
		    istringstream is( val ) ;
		    unsigned int uival = 0 ;
		    is >> uival ;
		    vals[attri] = (unsigned char)uival ;
		}
		stax = nc_put_att_uchar( _ncid, varid, new_name.c_str(),
					 NC_BYTE, num_vals, vals ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"File out netcdf, "
				 + "failed to write byte attribute "
				 + new_name ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
	    }
	    break ;
	case Attr_int16:
	    {
		// short
		short vals[num_vals] ;
		for( attri = 0; attri < num_vals; attri++ )
		{
		    string val = attrs.get_attr( attr, attri ) ;
		    istringstream is( val ) ;
		    short sval = 0 ;
		    is >> sval ;
		    vals[attri] = sval ;
		}
		stax = nc_put_att_short( _ncid, varid, new_name.c_str(),
					 NC_SHORT, num_vals, vals ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"File out netcdf, "
				 + "failed to write short attribute "
				 + new_name ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
	    }
	    break ;
	case Attr_uint16:
	    {
		// unsigned short
		// (needs to be big enough to store an unsigned short
		int vals[num_vals] ;
		for( attri = 0; attri < num_vals; attri++ )
		{
		    string val = attrs.get_attr( attr, attri ) ;
		    istringstream is( val ) ;
		    int ival = 0 ;
		    is >> ival ;
		    vals[attri] = ival ;
		}
		stax = nc_put_att_int( _ncid, varid, new_name.c_str(),
				       NC_INT, num_vals, vals ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"File out netcdf, "
				 + "failed to write unsinged short attribute "
				 + new_name ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
	    }
	    break ;
	case Attr_int32:
	    {
		// int
		int vals[num_vals] ;
		for( attri = 0; attri < num_vals; attri++ )
		{
		    string val = attrs.get_attr( attr, attri ) ;
		    istringstream is( val ) ;
		    int ival = 0 ;
		    is >> ival ;
		    vals[attri] = ival ;
		}
		stax = nc_put_att_int( _ncid, varid, new_name.c_str(),
				       NC_INT, num_vals, vals ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"File out netcdf, "
				 + "failed to write int attribute "
				 + new_name ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
	    }
	    break ;
	case Attr_uint32:
	    {
		// uint
		// needs to be big enough to store an unsigned int
		long vals[num_vals] ;
		for( attri = 0; attri < num_vals; attri++ )
		{
		    string val = attrs.get_attr( attr, attri ) ;
		    istringstream is( val ) ;
		    long lval = 0 ;
		    is >> lval ;
		    vals[attri] = lval ;
		}
		stax = nc_put_att_long( _ncid, varid, new_name.c_str(),
					NC_LONG, num_vals, vals ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"File out netcdf, "
				 + "failed to write byte attribute "
				 + new_name ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
	    }
	    break ;
	case Attr_float32:
	    {
		// float
		float vals[num_vals] ;
		for( attri = 0; attri < num_vals; attri++ )
		{
		    string val = attrs.get_attr( attr, attri ) ;
		    istringstream is( val ) ;
		    float fval = 0 ;
		    is >> fval ;
		    vals[attri] = fval ;
		}
		stax = nc_put_att_float( _ncid, varid, new_name.c_str(),
					 NC_FLOAT, num_vals, vals ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"File out netcdf, "
				 + "failed to write float attribute "
				 + new_name ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
	    }
	    break ;
	case Attr_float64:
	    {
		// double
		double vals[num_vals] ;
		for( attri = 0; attri < num_vals; attri++ )
		{
		    string val = attrs.get_attr( attr, attri ) ;
		    istringstream is( val ) ;
		    double dval = 0 ;
		    is >> dval ;
		    vals[attri] = dval ;
		}
		stax = nc_put_att_double( _ncid, varid, new_name.c_str(),
					  NC_DOUBLE, num_vals, vals ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"File out netcdf, "
				 + "failed to write double attribute "
				 + new_name ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
	    }
	    break ;
	case Attr_string:
	case Attr_url:
	    {
		// string
		string val = attrs.get_attr( attr, 0 ) ;
		for( attri = 1; attri < num_vals; attri++ )
		{
		    val += "\n" + attrs.get_attr( attr, attri ) ;
		}
		stax = nc_put_att_text( _ncid, varid, new_name.c_str(),
					val.length(), val.c_str());
		if( stax != NC_NOERR )
		{
		    string err = (string)"File out netcdf, "
				 + "failed to write string attribute "
				 + new_name ;
		    handle_error( stax, err, __FILE__, __LINE__ ) ;
		}
	    }
	    break ;
	case Attr_unknown:
	    {
		string err = (string)"File out netcdf, "
			     + "failed to write unknown type of attribute "
			     + new_name ;
		handle_error( stax, err, __FILE__, __LINE__ ) ;
	    }
	    break ;
    }
}

/** @brief write an OpeNDAP sequence to netcdf
 *
 * Currently we do not support this function. Instead we create a global
 * attribute stating that the sequence exists and that it has been elided
 *
 * @param b The OPeNDAP sequence to write out
 * @throws BESInternalError if there is a problem writing out the global
 * attribute
 */
void
FONcTransform::write_sequence( BaseType *b )
{
    string varname = FONcUtils::id2netcdf( embedded_name( b->name() ),
					   _name_prefix ) ;

    BESDEBUG( "fonc", "FONcTransform::write_sequence for var "
                      << varname << endl ) ;

    Sequence *s = dynamic_cast<Sequence *>(b) ;
    if( !s )
    {
	string s = (string)"File out netcdf, write_sequence passed a variable "
	           + "that is not a sequence" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }

    // for now we are simply going to add a global variable noting the
    // presence of the sequence, the name of the sequence, and that the
    // sequences has been elided.
    nc_redef( _ncid ) ;
    string val = (string)"The sequence " + varname
		 + " is a member of this dataset and has been elided." ;
    int stax = nc_put_att_text( _ncid, NC_GLOBAL, varname.c_str(),
				val.length(), val.c_str() ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"File out netcdf, "
		     + "failed to write string attribute for sequence "
		     + varname ;
	handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    nc_enddef( _ncid ) ;

    BESDEBUG( "fonc", "FONcTransform::write_sequence done for "
                      << varname << endl ) ;
}

/** @brief add an embedded object to the list of embedded objects
 *
 * This is provided for flattening out structures. For example, if there is
 * a structure Alpha that contains a structure Point that contains two
 * variables x and y, then we add the structures Alpha first, and then Point
 * second to this list.
 *
 * @param b The flattened object to add to the list
 */
void
FONcTransform::add_embedded( BaseType *b )
{
    _embedded.push_back( b ) ;
}

/** @brief remove an embedded object from the list
 *
 * Removes the last flattened object from the list
 *
 * @param b The flattened object to remove
 * @throws BESInternalError if the object to remove does not match the last
 * object in the list.
 */
void
FONcTransform::remove_embedded( BaseType *b )
{
    if( !_embedded.size() )
    {
	string err = (string)"Removing an embedded structure when "
		     + "there are none to remove" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    vector<BaseType *>::iterator e = _embedded.end() ;
    e-- ;
    if( (*e)->name() != b->name() )
    {
	string err = (string)"The embedded object to remove (" + b->name()
	             + " does not match the last object in the list ("
		     + (*e)->name() ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    _embedded.erase( e ) ;
}

/** @brief generate the embedded name given a variable name
 *
 * This method generates the flattened variable name given the list of
 * embedded objects. For example, if there is a structure Alpha that
 * contains a structure Point that contains the variable x, then this method
 * would create the name Alpha.Point.x.
 *
 * If the name specified is empty then we simply return the flattened
 * embedded objects. In the above example we would return Alpha.Point
 *
 * @param name name to append to the embedded objects names
 * @return the flattened variable name
 */
string
FONcTransform::embedded_name( const string &name )
{
    string new_name ;
    if( _embedded_set )
    {
	if( _embedded_name.empty() )
	{
	    new_name = name ;
	}
	else
	{
	    if( name.empty() )
	    {
		new_name = _embedded_name ;
	    }
	    else
	    {
		new_name = _embedded_name + EMBEDDED_SEPARATOR + name ;
	    }
	}
    }
    else
    {
	vector<BaseType *>::const_iterator i = _embedded.begin() ;
	vector<BaseType *>::const_iterator e = _embedded.end() ;
	bool isfirst = true ;
	for( ; i != e; i++ )
	{
	    if( !isfirst ) new_name += EMBEDDED_SEPARATOR ;
	    new_name += (*i)->name() ;
	    isfirst = false ;
	}

	// if name is empty then we just want the embedded part of the name not
	// including the variable name
	if( !name.empty() )
	{
	    if( !new_name.empty() ) new_name += EMBEDDED_SEPARATOR ;
	    new_name += name ;
	}
    }

    return new_name ;
}

void
FONcTransform::set_embedded( const string &ename )
{
    _embedded_name = ename ;
    _embedded_set = true ;
}

void
FONcTransform::unset_embedded()
{
    _embedded_name = "" ;
    _embedded_set = false ;
}

/** @brief handle any netcdf errors
 *
 * Looks up the netcdf error message associated with the provided netcdf
 * return value and throws an exception with that information appended to
 * the provided error message
 *
 * @param stax A netcdf return value, NC_NOERR if no error occurred
 * @param err A provided error message to begin the error message with
 * @param file The source code file name where the error was generated
 * @param line The source code line number where the error was generated
 * @throws BESInternalError if the return value represents a netcdf error
 */
void
FONcTransform::handle_error( int stax, string &err,
			     const string &file, int line )
{
    if( stax != NC_NOERR )
    {
	const char *nerr = nc_strerror( stax ) ;
	if( nerr )
	{
	    err += (string)": " + nerr ;
	}
	else
	{
	    err += (string)": unknown error" ;
	}
	throw BESInternalError( err, file, line ) ;
    }
}

void
FONcTransform::add_original_attr( BaseType *b, const string &orig )
{
    if( b )
    {
	AttrTable &attrs = b->get_attr_table() ;
	string val = attrs.get_attr( FONC_ORIGINAL_NAME, 0 ) ;
	if( val.empty() )
	{
	    attrs.append_attr( FONC_ORIGINAL_NAME, "String", orig ) ;
	}
    }
}

/** @brief dumps information about this transformation object for debugging
 * purposes
 *
 * Displays the pointer value of this instance plus instance data
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
    strm << BESIndent::LMarg << "dds: " ;
    if( _dds )
    {
	strm << endl ;
	BESIndent::Indent() ;
	strm << *_dds ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << "empty" << endl ;
    }
    strm << BESIndent::LMarg << "embedded names: " ;
    if( _embedded.size() )
    {
	BESIndent::Indent() ;
	vector<BaseType *>::const_iterator i = _embedded.begin() ;
	vector<BaseType *>::const_iterator e = _embedded.end() ;
	for( ; i != e; i++ )
	{
	    strm << BESIndent::LMarg << (*i)->name() << " of type "
	         << (*i)->type_name() << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << "none" << endl ;
    }
    strm << BESIndent::LMarg << "embedded name set: " << _embedded_name << endl;
    strm << BESIndent::LMarg << "embedded name set? " << _embedded_set << endl ;
    strm << BESIndent::LMarg << "grids: " << endl ;
    if( _grids.size() )
    {
	BESIndent::Indent() ;
	vector<FONcGrid *>::const_iterator gi = _grids.begin() ;
	vector<FONcGrid *>::const_iterator ge = _grids.end() ;
	for( ; gi != ge; gi++ )
	{
	    ostringstream display ;
	    display << "maps for " << (*gi)->grid->name() << ": " ;
	    vector<FONcMap *>::iterator gmi = (*gi)->maps.begin() ;
	    vector<FONcMap *>::iterator gme = (*gi)->maps.end() ;
	    bool isfirst = true ;
	    for( ; gmi != gme; gmi++ )
	    {
		if( !isfirst ) display << ", " ;
		display << (*gmi)->map->name() << "(" << (void *)(*gmi) << ")" ;
		isfirst = false ;
	    }
	    display << endl ;
	    strm << BESIndent::LMarg << display.str() ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << "none" << endl ;
    }
    strm << BESIndent::LMarg << "dims: " << endl ;
    if( _dims.size() )
    {
	BESIndent::Indent() ;
	vector<FONcDimSet *>::const_iterator di = _dims.begin() ;
	vector<FONcDimSet *>::const_iterator de = _dims.begin() ;
	for( ; di != de; di++ )
	{
	    strm << BESIndent::LMarg << "dimension set:" << endl ;
	    BESIndent::Indent() ;
	    strm << BESIndent::LMarg << "num dims = "
				     << (*di)->numdims << endl ;
	    strm << BESIndent::LMarg << "dim names =" ;
	    vector<string>::iterator dni = (*di)->ncdimnames.begin() ;
	    vector<string>::iterator dne = (*di)->ncdimnames.end() ;
	    for( ; dni != dne; dni++ )
	    {
		strm << " " << (*dni) ;
	    }
	    strm << endl ;
	    BESIndent::UnIndent() ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << "none" << endl ;
    }
    strm << BESIndent::LMarg << "maps: " << endl ;
    if( _maps.size() )
    {
	BESIndent::Indent() ;
	vector<FONcMap *>::const_iterator vi = _maps.begin() ;
	vector<FONcMap *>::const_iterator ve = _maps.end() ;
	for( ; vi != ve; vi++ )
	{
	    ostringstream display ;
	    display << "map " << (*vi)->map->name() << " shared by" ;
	    vector<string>::iterator gni = (*vi)->shared_by_grids.begin() ;
	    vector<string>::iterator gne = (*vi)->shared_by_grids.end() ;
	    bool isfirst = true ;
	    for( ; gni != gne; gni++ )
	    {
		if( isfirst ) display << ": " ;
		else display << ", " ;
		isfirst = false ;
		display << (*gni) ;
	    }
	    display << endl ;
	    strm << BESIndent::LMarg << display.str() ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << "none" << endl ;
    }
    BESIndent::UnIndent() ;
}

