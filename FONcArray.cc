// FONcArray.cc

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

#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcArray.h"
#include "FONcDim.h"
#include "FONcGrid.h"
#include "FONcMap.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

vector<FONcDim *> FONcArray::Dimensions ;

FONcArray::FONcArray( BaseType *b )
    : FONcBaseType(), _a( 0 ), _array_type( NC_NAT ), _ndims( 0 ),
      _actual_ndims( 0 ), _nelements( 1 ), _dim_ids( 0 ), _dim_sizes( 0 ),
      _str_data( 0 )
{
    _a = dynamic_cast<Array *>(b) ;
    if( !_a )
    {
	string s = (string)"File out netcdf, FONcArray was passed a "
		   + "variable that is not a DAP Array" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

FONcArray::~FONcArray()
{
    bool done = false ;
    while( !done )
    {
	vector<FONcDim *>::iterator i = _dims.begin() ;
	vector<FONcDim *>::iterator e = _dims.end() ;
	if( i == e )
	{
	    done = true ;
	}
	else
	{
	    // These are the FONc types, not the actual ones
	    FONcDim *d = (*i) ;
	    d->decref() ;
	    _dims.erase( i ) ;
	}
    }
}

void
FONcArray::convert( vector<string> embed )
{
    FONcBaseType::convert( embed ) ;
    _varname = FONcUtils::gen_name( embed, _varname, _orig_varname ) ;
    BESDEBUG( "fonc", "FONcArray::convert - converting array "
                      << _varname << endl ) ;

    _array_type = FONcUtils::get_nc_type( _a->var() ) ;
    _ndims = _a->dimensions() ;
    _actual_ndims = _ndims ;
    if( _array_type == NC_CHAR )
    {
	// if we have array of strings then we need to add the string length
	// dimension, so add one more to ndims
	_ndims++ ;
    }

    _dim_ids = new int[_ndims] ;
    _dim_sizes = new int[_ndims] ;

    Array::Dim_iter di = _a->dim_begin() ;
    Array::Dim_iter de = _a->dim_end() ;
    int dimnum = 0 ;
    for( ; di != de; di++ )
    {
	int size = _a->dimension_size( di, true ) ;
	_dim_sizes[dimnum] = size ;
	_nelements *= size ;

	// See if this dimension has already been defined. If it has the
	// same name and same size as another dimension, then it is a
	// shared dimension. Create it only once and share the FONcDim
	FONcDim *use_dim = find_dim( _a->dimension_name( di ), size ) ;
	_dims.push_back( use_dim ) ;
	dimnum++ ;
    }

    // If this array has a single dimension, and the name of the array
    // and the name of that dimension are the same, then this array
    // might be used as a map for a grid defined elsewhere.
    if( !FONcGrid::InGrid && _actual_ndims == 1 &&
        _a->name() == _a->dimension_name( _a->dim_begin() ) )
    {
	FONcGrid::Maps.push_back( new FONcMap( this ) ) ;
    }

    BESDEBUG( "fonc", "FONcArray::convert - done converting array "
                      << _varname << endl << *this << endl ) ;
}

FONcDim *
FONcArray::find_dim( const string &name, int size )
{
    FONcDim *ret_dim = 0 ;
    vector<FONcDim *>::iterator i = FONcArray::Dimensions.begin() ;
    vector<FONcDim *>::iterator e = FONcArray::Dimensions.end() ;
    for( ; i != e && !ret_dim; i++ )
    {
	if( (*i)->name() == name )
	{
	    if( (*i)->size() == size )
	    {
		ret_dim = (*i) ;
	    }
	    else
	    {
		string err = (string)"fileout_netcdf:dimension found "
			     + "with the same name, but different size" ;
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	    }
	}
    }
    if( !ret_dim )
    {
	ret_dim = new FONcDim( name, size ) ;
	FONcArray::Dimensions.push_back( ret_dim ) ;
    }
    else
    {
	ret_dim->incref() ;
    }
    return ret_dim ;
}

void
FONcArray::define( int ncid )
{
    if( !_defined )
    {
	BESDEBUG( "fonc", "FONcArray::define - defining array "
			  << _varname << endl ) ;

	vector<FONcDim *>::iterator i = _dims.begin() ;
	vector<FONcDim *>::iterator e = _dims.end() ;
	int dimnum = 0 ;
	for( ; i != e; i++ )
	{
	    FONcDim *fd = *i ;
	    fd->define( ncid ) ;
	    _dim_ids[dimnum] = fd->dimid() ;
	    dimnum++ ;
	}

	if( _array_type != NC_CHAR )
	{
	    int stax = nc_def_var( ncid, _varname.c_str(), _array_type,
				   _ndims, _dim_ids, &_varid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"fileout.netcdf - "
			     + "Failed to define variable "
			     + _varname ;
		FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
	    }
	}
	else
	{
	    // special case for string data. We need to add another dimension
	    // for the length of the string.

	    // get the data from the dap array
	    int array_length = _a->length() ;
	    _str_data = new string[array_length] ;
	    _a->buf2val( (void**)&_str_data ) ;

	    // determine the max length of the strings
	    int max_length = 0 ;
	    for( int i = 0; i < array_length; i++ )
	    {
		if( _str_data[i].length() > max_length )
		{
		    max_length = _str_data[i].length() ;
		}
	    }
	    max_length++ ;
	    _dim_sizes[_ndims-1] = max_length ;

	    // create the string dimension with the max length
	    string lendim_name = _varname + "_len" ;
	    int this_dimension = 0 ;
	    int stax = nc_def_dim( ncid, lendim_name.c_str(),
				   max_length, &this_dimension ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"fileout.netcdf - "
			     + "Failed to define string dimension "
			     + lendim_name ;
		FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
	    }
	    _dim_ids[_ndims-1] = this_dimension ;

	    stax = nc_def_var( ncid, _varname.c_str(), _array_type,
			       _ndims, _dim_ids, &_varid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"fileout.netcdf - "
			     + "Failed to define array variable "
			     + _varname ;
		FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
	    }
	}

	FONcAttributes::add_attributes( ncid, _varid, _a ) ;
	FONcAttributes::add_original_name( ncid, _varid,
					   _varname, _orig_varname ) ;

	_defined = true ;

	BESDEBUG( "fonc", "FONcArray::define - done defining array "
			  << _varname << endl ) ;
    }
}

void
FONcArray::write( int ncid )
{
    BESDEBUG( "fonc", "FONcArray::write for var " << _varname << endl ) ;

    ncopts = NC_VERBOSE ;
    int stax = NC_NOERR ;

    int varid = 0 ;
    if( _array_type != NC_CHAR )
    {
	// create array to hold data hyperslab
	switch( _array_type )
	{
	    case NC_BYTE:
		{
		    unsigned char *data = new unsigned char[_nelements] ;
		    _a->buf2val( (void**)&data ) ;
		    stax = nc_put_var_uchar( ncid, _varid, data ) ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of bytes for "
				     + _varname ;
			FONcUtils::handle_error( stax, err,
						 __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    case NC_SHORT:
		{
		    short *data = new short [_nelements] ;
		    _a->buf2val( (void**)&data ) ;
		    int stax = nc_put_var_short( ncid, _varid, data ) ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of shorts for "
				     + _varname ;
			FONcUtils::handle_error( stax, err,
						 __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    case NC_INT:
		{
		    int *data = new int[_nelements] ;
		    _a->buf2val( (void**)&data ) ;
		    int stax = nc_put_var_int( ncid, _varid, data ) ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of ints for "
				     + _varname ;
			FONcUtils::handle_error( stax, err,
						 __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    case NC_FLOAT:
		{
		    float *data = new float[_nelements] ;
		    _a->buf2val( (void**)&data ) ;
		    int stax = nc_put_var_float( ncid, _varid, data ) ;
		    ncopts = NC_VERBOSE ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of floats for "
				     + _varname ;
			FONcUtils::handle_error( stax, err,
						 __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    case NC_DOUBLE:
		{
		    double *data = new double[_nelements] ;
		    _a->buf2val( (void**)&data ) ;
		    int stax = nc_put_var_double( ncid, _varid, data ) ;
		    if( stax != NC_NOERR )
		    {
			string err = (string)"fileout.netcdf - "
				     + "Failed to create array of doubles for "
				     + _varname ;
			FONcUtils::handle_error( stax, err,
						 __FILE__, __LINE__ ) ;
		    }
		    delete [] data ;
		}
		break ;
	    default:
		string err = (string)"Failed to transform array of unknown "
			     + "type in file out netcdf" ;
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	} ;
    }
    else
    {
	// special case for string data. Could have put this in the
	// switch, but it's pretty big
	size_t var_count[_ndims] ;
	size_t var_start[_ndims] ;
	int dim = 0 ;
	for( dim = 0; dim < _ndims; dim++ )
	{
	    // the count for each of the dimensions will always be 1 except
	    // for the string length dimension
	    var_count[dim] = 1 ;

	    // the start for each of the dimensions will start at 0. We will
	    // bump this up in the while loop below
	    var_start[dim] = 0 ;
	}

	for( int element = 0; element < _nelements; element++ )
	{
	    var_count[_ndims-1] = _str_data[element].size() + 1 ;
	    var_start[_ndims-1] = 0 ;

	    // write out the string
	    int stax = nc_put_vara_text( ncid, _varid, var_start, var_count, 
				         _str_data[element].c_str() ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"fileout.netcdf - "
			     + "Failed to create array of strings for "
			     + _varname ;
		FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
	    }

	    // bump up the start.
	    if( element+1 < _nelements )
	    {
		bool done = false ;
		dim = _ndims-2 ;
		while( !done )
		{
		    var_start[dim] = var_start[dim] + 1 ;
		    if( var_start[dim] == _dim_sizes[dim] )
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
	delete [] _str_data ;
	_str_data = 0 ;
    }

    BESDEBUG( "fonc", "FONcTransform::write_array done for "
                      << _varname << endl ) ;
}

string
FONcArray::name()
{
    return _a->name() ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcArray::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcArray::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _varname << endl ;
    strm << BESIndent::LMarg << "ndims = " << _ndims << endl ;
    strm << BESIndent::LMarg << "actual ndims = " << _actual_ndims << endl ;
    strm << BESIndent::LMarg << "nelements = " << _nelements << endl ;
    if( _dims.size() )
    {
	strm << BESIndent::LMarg << "dimensions:" << endl ;
	BESIndent::Indent() ;
	vector<FONcDim *>::const_iterator i = _dims.begin() ;
	vector<FONcDim *>::const_iterator e = _dims.end() ;
	for( ; i != e; i++ )
	{
	    (*i)->dump( strm ) ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "dimensions: none" << endl ;
    }
    BESIndent::UnIndent() ;
}

