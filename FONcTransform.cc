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

#include "FONcTransform.h"

#include <DDS.h>
#include <Structure.h>
#include <Array.h>
#include <BESDebug.h>
#include <BESInternalError.h>

nc_type
FONcTransform::get_nc_type( BaseType *element )
{
    nc_type x_type = NC_NAT ; // the constant ncdf uses to define simple type

    string var_type = element->type_name() ;
    BESDEBUG( "fonc", "FONcTransform::get_nc_type for "
                      << var_type << endl )
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

    BESDEBUG( "fonc", "FONcTransform::get_nc_type returning "
                      << x_type << endl )
    return x_type ;
}

void
FONcTransform::write_structure( BaseType* b, int ncid )
{
    Structure *s = (Structure*)b ;
    string myname = s->name() ;
    BESDEBUG( "fonc", "FONcTransform::write_structure for "
                      << myname << endl )
    Constructor::Vars_iter vi = s->var_begin() ;
    Constructor::Vars_iter ve = s->var_end() ;
    for( ; vi != ve; vi++ )
    {
	BaseType *bt = *vi ;
	if( bt->send_p() )
	{
	    string new_name = myname + string( "__" ) + bt->name() ;
	    BESDEBUG( "fonc", "FONcTransform::write_structure new name: "
			      << new_name << endl )
	    bt->set_name( new_name ) ;
	    switch( bt->type() )
	    {
		case dods_byte_c:
		case dods_str_c:
		case dods_url_c:
		case dods_int16_c:
		case dods_uint16_c:
		case dods_int32_c:
		case dods_uint32_c:
		case dods_float32_c:
		case dods_float64_c:
		    write_var( bt, ncid ) ;
		    break ;
		case dods_grid_c:
		    write_grid( bt, ncid ) ;
		    break ;
		case dods_array_c:
		    write_array( bt, ncid ) ;
		    break ;
		case dods_structure_c:
		    write_structure( bt, ncid ) ;
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
	                      << "writing " << new_name << endl )
	}
    }
}

void
FONcTransform::write_grid( BaseType* b, int ncid )
{
    string err = "Grid code mapping is not implemented in file out netcdf" ;
    throw BESInternalError( err, __FILE__, __LINE__ ) ;
}

void
FONcTransform::write_array( BaseType* b, int ncid )
{
    Array *a = dynamic_cast<Array *>( b ) ;
    if( !a )
    {
	string s = (string)"File out netcdf, write_array passed a variable "
	           + "that is not an array" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
    BESDEBUG( "fonc", "FONcTransform::write_array for array "
                      << a->name() << endl )
    int varid ;
    nc_redef( ncid ) ;
    int *dimensions = new int[a->dimensions()] ;
    int j = 0 ;
    int stax = NC_NOERR ;

    Array::Dim_iter di = a->dim_begin() ;
    Array::Dim_iter de = a->dim_end() ;
    int number_of_elements = 1 ;
    for( ; di != de; di++ )
    {
	int this_dimension ;
	int this_dimension_size = a->dimension_size( di ) ;
	const char *this_dimension_name = a->dimension_name( di ).c_str() ;

	// check to see if the dimension is already defined
	if( nc_inq_dimid( ncid, this_dimension_name, &this_dimension ) != NC_NOERR )
	{
	    // The dimension does not exist add it...
	    stax = nc_def_dim( ncid, this_dimension_name,
			       this_dimension_size, &this_dimension ) ;
	    if( stax != NC_NOERR )
	    {
		const char * nerr = nc_strerror( stax ) ;
		string err = (string)"fileout.netcdf - "
			     + "Failed to define dimension "
			     + this_dimension_name ;
		if( nerr )
		{
		    err += ": " + (string)nerr ;
		}
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	    }
	}
	dimensions[j] = this_dimension ;
	j++ ;
	number_of_elements *= this_dimension_size ;
    }

    nc_type array_type = get_nc_type( a->var() ) ;
    ncopts = NC_VERBOSE ;
    stax = nc_def_var( ncid, a->name().c_str(), array_type,
			   a->dimensions(), dimensions, &varid ) ;
    if( stax != NC_NOERR )
    {
	const char * nerr = nc_strerror( stax ) ;
	string err = (string)"fileout.netcdf - "
		     + "Failed to define variable "
		     + a->name() ;
	if( nerr )
	{
	    err += ": " + (string)nerr ;
	}
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    nc_enddef( ncid ) ;
    delete [] dimensions ;

    // create array to hold data hyperslab
    switch( array_type )
    {
	case NC_BYTE:
	    {
		unsigned char *data = new unsigned char[number_of_elements] ;
		a->buf2val( (void**)&data ) ;
		stax = nc_put_var_uchar( ncid, varid, data ) ;
		if( stax != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stax ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to create array of bytes for "
				 + a->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete [] data ;
	    }
	    break ;
	case NC_CHAR:
	    {
		string *data = new string[a->length()] ;
		a->buf2val( (void**)&data ) ;
		char *char_data = new char[a->length()] ;
		for( int g = 0; g < a->length(); g++ )
		{
		    if( data[g].length() > 0 )
			char_data[g] = data[g][0] ;
		    else
			char_data[g] = '\0' ;
		}
		int stat = nc_put_var_text( ncid, varid, char_data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stax ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to create array of char for "
				 + a->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete [] data ;
		delete [] char_data ;
	    }
	    break ;
	case NC_SHORT:
	    {
		short *data = new short [number_of_elements] ;
		a->buf2val( (void**)&data ) ;
		int stat = nc_put_var_short( ncid, varid, data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stax ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to create array of shorts for "
				 + a->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete [] data ;
	    }
	    break ;
	case NC_INT:
	    {
		int *data = new int[number_of_elements] ;
		a->buf2val( (void**)&data ) ;
		int stat = nc_put_var_int( ncid, varid, data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stax ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to create array of ints for "
				 + a->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete [] data ;
	    }
	    break ;
	case NC_FLOAT:
	    {
		float *data = new float[number_of_elements] ;
		a->buf2val( (void**)&data ) ;
		int stat = nc_put_var_float( ncid, varid, data ) ;
		ncopts = NC_VERBOSE ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stax ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to create array of floats for "
				 + a->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete [] data ;
	    }
	    break ;
	case NC_DOUBLE:
	    {
		double *data = new double[number_of_elements] ;
		a->buf2val( (void**)&data ) ;
		int stat = nc_put_var_double( ncid, varid, data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stax ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to create array of doubles for "
				 + a->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete [] data ;
	    }
	    break ;
	default:
	    string err = "Failed to transform unknown type in file out netcdf" ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	    break ;
    } ;

    BESDEBUG( "fonc", "FONcTransform::write_array done for "
                      << a->name() << endl )
}

void
FONcTransform::write_var( BaseType* b, int ncid )
{
    BESDEBUG( "fonc", "FONcTransform::write_var for var "
                      << b->name() << endl )
    int varid ;
    static size_t var_index[] = {0} ;
    nc_type var_type = get_nc_type( b ) ;
    nc_redef( ncid ) ;
    nc_def_var( ncid, b->name().c_str(), var_type, 0, NULL, &varid ) ;
    nc_enddef( ncid ) ;
    switch( var_type )
    {
	case NC_BYTE:
	    {
		unsigned char *data = new unsigned char ;
		b->buf2val( (void**)&data ) ;
		int stat = nc_put_var1_uchar( ncid, varid, var_index, data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stat ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write byte data for "
				 + b->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_CHAR:
	    {
		string *data = new string ;
		b->buf2val( (void**)&data ) ;
		char char_data ;
		if( data->length() > 0 )
		{
		    char_data = (*data)[0] ;
		}
		else
		{
		    char_data = '\0' ;
		}
		int stat = nc_put_var1_text( ncid, varid,
					     var_index, &char_data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stat ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write text data for "
				 + b->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_SHORT:
	    {
		short *data = new short ;
		b->buf2val( (void**)&data ) ;
		int stat = nc_put_var1_short( ncid, varid, var_index, data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stat ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write short data for "
				 + b->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_INT:
	    {
		int *data = new int ;
		b->buf2val( (void**)&data ) ;
		int stat = nc_put_var1_int( ncid, varid, var_index, data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stat ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write int data for "
				 + b->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_FLOAT:
	    {
		float *data = new float ;
		b->buf2val( (void**)&data ) ;
		int stat = nc_put_var1_float( ncid, varid, var_index, data ) ;
		ncopts = NC_VERBOSE ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stat ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write float data for "
				 + b->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	case NC_DOUBLE:
	    {
		double *data = new double ;
		b->buf2val( (void**)&data ) ;
		int stat = nc_put_var1_double( ncid, varid, var_index, data ) ;
		if( stat != NC_NOERR )
		{
		    const char *nerr = nc_strerror( stat ) ;
		    string err = (string)"fileout.netcdf - "
				 + "Failed to write double data for "
				 + b->name() ;
		    if( nerr )
		    {
			err += ": " + (string)nerr ;
		    }
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		}
		delete data ;
	    }
	    break ;
	default:
	    {
		string err = (string)"fileout.netcdf - "
			     + "Unable to write netcdf data type "
			     + b->type_name() + " for " + b->name() ;
		throw BESInternalError( err, __FILE__, __LINE__ ) ;
	    }
	    break ;
    }
    BESDEBUG( "fonc", "FONcTransform::write_var done for "
                      << b->name() << endl )
}

int
FONcTransform::create_local_nc( DDS *dds, char* localfile )
{
    int ncid = nccreate( localfile, NC_CLOBBER ) ;

    DDS::Vars_iter vi = dds->var_begin() ;
    DDS::Vars_iter ve = dds->var_end() ;
    for( ; vi != ve; vi++ )
    {
	if( (*vi)->send_p() )
	{
	    BaseType *v = *vi ;
	    //v->read("d1") ;
	    switch( v->type() )
	    {
		case dods_byte_c:
		case dods_str_c:
		case dods_url_c:
		case dods_int16_c:
		case dods_uint16_c:
		case dods_int32_c:
		case dods_uint32_c:
		case dods_float32_c:
		case dods_float64_c:
		    write_var( v, ncid ) ;
		    break ;
		case dods_grid_c:
		    write_grid( v, ncid ) ;
		    break ;
		case dods_array_c:
		    write_array( v, ncid ) ;
		    break ;
		case dods_structure_c:
		    write_structure( v, ncid ) ;
		    break ;
		default:
		    string err = (string)"file out netcdf, unable to write "
		                 + "unknown variable type" ;
		    throw BESInternalError( err, __FILE__, __LINE__ ) ;
		    break ;
	    }
	}
    }

    ncclose( ncid ) ;

    return 1 ;
}

int
FONcTransform::copy_all_attributes( const string &var_name,
					const string &source_file,
					const string &target_file )
{
    int  status ;                        /* error status */
    int  ncid, ncidt ;                   /* netCDF ID */
    int  var_id, var_idt ;               /* variable ID */
    int var_natts ;                      /* number of attributes */

    status = nc_open( source_file.c_str(), NC_NOWRITE, &ncid ) ;
    if( status != NC_NOERR )
    {
	const char *nerr = nc_strerror( status ) ;
	string err = (string)"fileout.netcdf - "
		     + "Failed to copy attributes, unable to open file for "
		     + var_name ;
	if( nerr )
	{
	    err += ": " + (string)nerr ;
	}
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    status = nc_inq_varid( ncid, var_name.c_str(), &var_id ) ;
    if( status != NC_NOERR )
    {
	const char *nerr = nc_strerror( status ) ;
	string err = (string)"fileout.netcdf - "
		     + "Failed to copy attributes, unable to find variable "
		     + var_name ;
	if( nerr )
	{
	    err += ": " + (string)nerr ;
	}
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    status = nc_inq_varnatts( ncid, var_id, &var_natts ) ;
    if( status != NC_NOERR )
    {
	const char *nerr = nc_strerror( status ) ;
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
	status = nc_inq_attname( ncid, var_id, attnum, att_name ) ;
	if( status != NC_NOERR )
	{
	    const char *nerr = nc_strerror( status ) ;
	    string err = (string)"fileout.netcdf - "
			 + "Failed to copy attributes, unable to find attr "
			 + att_name + " for " + var_name ;
	    if( nerr )
	    {
		err += ": " + (string)nerr ;
	    }
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	status = nc_open( target_file.c_str(), NC_WRITE, &ncidt ) ;
	if( status != NC_NOERR )
	{
	    const char *nerr = nc_strerror( status ) ;
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
	status = nc_inq_varid( ncidt, var_name.c_str(), &var_idt ) ;
	if( status != NC_NOERR )
	{
	    const char *nerr = nc_strerror( status ) ;
	    string err = (string)"fileout.netcdf - "
			 + "Failed to copy attributes, unable to find var "
			 + var_name + " in target file" ;
	    if( nerr )
	    {
		err += ": " + (string)nerr ;
	    }
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	status = nc_redef( ncidt ) ;
	if( status != NC_NOERR )
	{
	    const char *nerr = nc_strerror( status ) ;
	    string err = (string)"fileout.netcdf - "
			 + "Failed to copy attributes, "
			 + "unable to set define mode for var " + var_name ;
	    if( nerr )
	    {
		err += ": " + (string)nerr ;
	    }
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	status = nc_copy_att( ncid, var_id, att_name, ncidt, var_idt ) ;
	if( status != NC_NOERR )
	{
	    const char *nerr = nc_strerror( status ) ;
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
