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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <sstream>

using std::ostringstream ;

#include "FONcTransform.h"

#include <DDS.h>
#include <Structure.h>
#include <Array.h>
#include <BESDebug.h>
#include <BESInternalError.h>

FONcTransform::FONcTransform( DDS *dds, const string &localfile )
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
    _dds = dds ;
    _localfile = localfile ;

    int stax = nc_create( _localfile.c_str(), NC_CLOBBER, &_ncid ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"File out netcdf, "
	             + "unable to open file " + _localfile ;
	handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
}

FONcTransform::~FONcTransform()
{
}

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
		    default:
			string err = (string)"file out netcdf, unable to write "
				     + "unknown variable type" ;
			throw BESInternalError( err, __FILE__, __LINE__ ) ;
			break ;
		}
	    }
	}
    }
    catch( BESError &e )
    {
	nc_close( _ncid ) ;
	throw e ;
    }

    nc_close( _ncid ) ;

    return 1 ;
}

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
                      << myname << endl )

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
				  << "writing " << bt->name() << endl )
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
				  << "writing " << bt->name() << endl )
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
                      << myname << endl )
}

void
FONcTransform::write_grid( BaseType* b )
{
    string err = "Grid code mapping is not implemented in file out netcdf" ;
    throw BESInternalError( err, __FILE__, __LINE__ ) ;
}

void
FONcTransform::write_array( BaseType* b )
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
                      << varname << endl )

    nc_type array_type = get_nc_type( a->var() ) ;
    int varid = 0 ;
    int ndims = a->dimensions() ;
    if( array_type == NC_CHAR )
    {
	// if we have array of strings then we need to add the string length
	// dimension, so add one more to ndims
	ndims++ ;
    }

    nc_redef( _ncid ) ;

    int dims[ndims] ;
    int dim_sizes[ndims] ;
    int dim_num = 0 ;
    int nelements = 1 ;
    int stax = NC_NOERR ;

    Array::Dim_iter di = a->dim_begin() ;
    Array::Dim_iter de = a->dim_end() ;
    for( ; di != de; di++ )
    {
	int this_dimension ;
	int this_dimension_size = a->dimension_size( di ) ;
	string dimname_s = a->dimension_name( di ) ;
	if( dimname_s.empty() )
	{
	    ostringstream dimname_strm ;
	    dimname_strm << varname << "_dim" << dim_num+1 ;
	    dimname_s = dimname_strm.str() ;
	}
	const char *this_dimension_name = dimname_s.c_str() ;

	// check to see if the dimension is already defined
	stax = nc_inq_dimid( _ncid, this_dimension_name, &this_dimension ) ;
	if( stax != NC_NOERR )
	{
	    // The dimension does not exist add it...
	    stax = nc_def_dim( _ncid, this_dimension_name,
			       this_dimension_size, &this_dimension ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"fileout.netcdf - "
			     + "Failed to define dimension "
			     + this_dimension_name ;
		handle_error( stax, err, __FILE__, __LINE__ ) ;
	    }
	}
	dims[dim_num] = this_dimension ;
	dim_sizes[dim_num] = this_dimension_size ;
	dim_num++ ;
	nelements *= this_dimension_size ;
    }

    ncopts = NC_VERBOSE ;
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
	nc_enddef( _ncid ) ;

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
		break ;
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
	dim_sizes[dim_num] = max_length ;

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
	dims[dim_num] = this_dimension ;

	stax = nc_def_var( _ncid, varname.c_str(), array_type,
			   ndims, dims, &varid ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"fileout.netcdf - "
			 + "Failed to define variable "
			 + varname ;
	    handle_error( stax, err, __FILE__, __LINE__ ) ;
	}
	nc_enddef( _ncid ) ;

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
	    const char *val = data[element].c_str() ;
	    var_count[ndims-1] = strlen( val ) + 1 ;
	    var_start[ndims-1] = 0 ;

	    // write out the string
	    stax = nc_put_vara_text( _ncid, varid, var_start, var_count, val ) ;
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
                      << varname << endl )
}

void
FONcTransform::write_str( BaseType *b )
{
    string varname = embedded_name( b->name() ) ;

    BESDEBUG( "fonc", "FONcTransform::write_str for var "
                      << varname << endl )
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
    const char *val = data->c_str() ;

    string dimname = varname + "_len" ;
    int stax = nc_def_dim( _ncid, dimname.c_str(), strlen( val )+1, &chid ) ;
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

    nc_enddef( _ncid ) ;

    var_count[0] = strlen( val) + 1 ;
    var_start[0] = 0 ;
    stax = nc_put_vara_text( _ncid, varid, var_start, var_count, val ) ;
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
                      << varname << endl )
}

void
FONcTransform::write_var( BaseType* b )
{
    string varname = embedded_name( b->name() ) ;

    BESDEBUG( "fonc", "FONcTransform::write_var for var "
                      << varname << endl )
    int varid ;
    size_t var_index[] = {0} ;
    nc_type var_type = get_nc_type( b ) ;
    nc_redef( _ncid ) ;
    nc_def_var( _ncid, varname.c_str(), var_type, 0, NULL, &varid ) ;
    nc_enddef( _ncid ) ;
    switch( var_type )
    {
	case NC_BYTE:
	    {
		unsigned char *data = new unsigned char ;
		b->buf2val( (void**)&data ) ;
		int stax =
		    nc_put_var1_uchar( _ncid, varid, var_index, data ) ;
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
		int stax =
		    nc_put_var1_short( _ncid, varid, var_index, data ) ;
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
		int stax = nc_put_var1_int( _ncid, varid, var_index, data ) ;
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
		int stax =
		    nc_put_var1_float( _ncid, varid, var_index, data ) ;
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
		int stax =
		    nc_put_var1_double( _ncid, varid, var_index, data ) ;
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
	    break ;
    }
    BESDEBUG( "fonc", "FONcTransform::write_var done for "
                      << varname << endl )
}

void
FONcTransform::add_embedded( BaseType *b )
{
    _embedded.push_back( b ) ;
}

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
    _embedded.erase( e ) ;
}

string
FONcTransform::embedded_name( const string &name )
{
    string new_name ;
    vector<BaseType *>::const_iterator i = _embedded.begin() ;
    vector<BaseType *>::const_iterator e = _embedded.end() ;
    for( ; i != e; i++ )
    {
	new_name += (*i)->name() + "." ;
    }
    new_name += name ;

    return new_name ;
}

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
    BESIndent::UnIndent() ;
}

/*
int
FONcTransform::copy_all_attributes( const string &var_name,
					const string &source_file,
					const string &target_file )
{
    int  stax ;                        //
    int  ncid, ncidt ;                   //
    int  var_id, var_idt ;               //
    int var_natts ;                      //

    stax = nc_open( source_file.c_str(), NC_NOWRITE, &ncid ) ;
    if( stax != NC_NOERR )
    {
	const char *nerr = nc_strerror( stax ) ;
	string err = (string)"fileout.netcdf - "
		     + "Failed to copy attributes, unable to open file for "
		     + var_name ;
	if( nerr )
	{
	    err += ": " + (string)nerr ;
	}
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    stax = nc_inq_varid( ncid, var_name.c_str(), &var_id ) ;
    if( stax != NC_NOERR )
    {
	const char *nerr = nc_strerror( stax ) ;
	string err = (string)"fileout.netcdf - "
		     + "Failed to copy attributes, unable to find variable "
		     + var_name ;
	if( nerr )
	{
	    err += ": " + (string)nerr ;
	}
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    stax = nc_inq_varnatts( ncid, var_id, &var_natts ) ;
    if( stax != NC_NOERR )
    {
	const char *nerr = nc_strerror( stax ) ;
	string err = (string)"fileout.netcdf - "
		     + "Failed to copy attributes, unable to find attrs for "
		     + var_name ;
	if( nerr )
	{
	    err += ": " + (string)nerr ;
	}
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    for( int attnum = 0; attnum < var_natts; attnum++ )
    {
	char att_name[200] ;
	stax = nc_inq_attname( ncid, var_id, attnum, att_name ) ;
	if( stax != NC_NOERR )
	{
	    const char *nerr = nc_strerror( stax ) ;
	    string err = (string)"fileout.netcdf - "
			 + "Failed to copy attributes, unable to find attr "
			 + att_name + " for " + var_name ;
	    if( nerr )
	    {
		err += ": " + (string)nerr ;
	    }
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	stax = nc_open( target_file.c_str(), NC_WRITE, &ncidt ) ;
	if( stax != NC_NOERR )
	{
	    const char *nerr = nc_strerror( stax ) ;
	    string err = (string)"fileout.netcdf - "
			 + "Failed to copy attributes, "
			 + "unable to open target file "
			 + target_file ;
	    if( nerr )
	    {
		err += ": " + (string)nerr ;
	    }
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	stax = nc_inq_varid( ncidt, var_name.c_str(), &var_idt ) ;
	if( stax != NC_NOERR )
	{
	    const char *nerr = nc_strerror( stax ) ;
	    string err = (string)"fileout.netcdf - "
			 + "Failed to copy attributes, unable to find var "
			 + var_name + " in target file" ;
	    if( nerr )
	    {
		err += ": " + (string)nerr ;
	    }
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	stax = nc_redef( ncidt ) ;
	if( stax != NC_NOERR )
	{
	    const char *nerr = nc_strerror( stax ) ;
	    string err = (string)"fileout.netcdf - "
			 + "Failed to copy attributes, "
			 + "unable to set define mode for var " + var_name ;
	    if( nerr )
	    {
		err += ": " + (string)nerr ;
	    }
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	stax = nc_copy_att( ncid, var_id, att_name, ncidt, var_idt ) ;
	if( stax != NC_NOERR )
	{
	    const char *nerr = nc_strerror( stax ) ;
	    string err = (string)"fileout.netcdf - "
			 + "Failed to copy attributes, unable to copy attr "
			 + att_name + " for " + var_name ;
	    if( nerr )
	    {
		err += ": " + (string)nerr ;
	    }
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	nc_close( ncidt ) ;
    }
    nc_close( ncid ) ;

    return 0 ;
}
*/

