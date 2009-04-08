// FONcTransform.h

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

#ifndef FONcGridTransfrom_h_
#define FONcGridTransfrom_h_ 1

#include <netcdf.h>

#include <string>
#include <vector>
#include <map>

using std::string ;
using std::vector ;
using std::map ;

#include <DDS.h>

using namespace::libdap ;

#include <BESObj.h>
#include <BESDataHandlerInterface.h>

/** @brief Transformation object that converts an OPeNDAP DataDDS to a
 * netcdf file
 *
 * This class transforms each variable of the DataDDS to a netcdf file. For
 * more information on the transformation please refer to the OpeNDAP
 * documents wiki.
 */
class FONcTransform : public BESObj
{
private:
    int				_ncid ;
    DDS				*_dds ;
    string			_localfile ;
    string			_name_prefix ;
    vector<BaseType *>		_embedded ;
    bool			_embedded_set ;
    string			_embedded_name ;
    bool			_doing_grids ;

    // private class representing a grid map, or possible grid map
    class FONcMap
    {
    private:
	FONcMap() : map( 0 ), dimid( 0 ) {}
    public:
	Array *map ;
	string embedded_name ;
	vector<string> shared_by_grids ;
	int dimid ;
	bool written ;

	FONcMap( const string &gridname,
		 const string &ename,
		 Array *a )
	    : map( a ), embedded_name( ename), dimid( 0 ), written( false )
	{
	    shared_by_grids.push_back( gridname ) ;
	}
	FONcMap( Array *a, int id )
	    : map( a ), dimid( id ), written( true ) { }

	bool compare( Array *tomap ) ;
    } ;

    vector<FONcMap *> _maps ;

    // private class that represents a Grid object
    class FONcGrid
    {
    private:
	FONcGrid() : grid( 0 ) {}
    public:
	FONcGrid( const string &ename, Grid *g )
	    : grid( g ), embedded_name( ename ) {}
	Grid *grid ;
	string embedded_name ;
	vector<FONcMap *> maps ;
    } ;

    vector<FONcGrid *>		_grids ;

    nc_type			get_nc_type( BaseType *element ) ;
    void			write_structure( BaseType* b ) ;
    void			write_array( BaseType* b, int dimids[] = NULL );
    int				write_array( Array *a, nc_type array_type,
					     int nelements,
					     int ndims, int dims[],
					     int dim_sizes[] ) ;
    void			write_var( BaseType* b ) ;
    void			write_str( BaseType* b ) ;
    void			add_attributes( int varid, BaseType *b ) ;
    void			addattrs( int varid, BaseType *b,
					  const string &var_name ) ;
    void			addattrs( int varid, AttrTable &attrs,
					  const string &var_name,
					  const string &prepend_attr ) ;
    void			addattrs( int varid, const string &var_name,
					  AttrTable &attrs,
					  AttrTable::Attr_iter &attr,
					  const string &prepend_attr ) ;

    void			add_embedded( BaseType *b ) ;
    void			remove_embedded( BaseType *b ) ;
    string			embedded_name( const string &name ) ;
    void			set_embedded( const string &ename ) ;
    void			unset_embedded( ) ;

    void			handle_error( int stax, string &err,
					      const string &file, int line ) ;

    void			write_grid( BaseType* b ) ;
    void			write_grids() ;

    void			write_sequence( BaseType *b ) ;

    string			id2netcdf( string in ) ;
    void			add_original_attr( BaseType *b,
						   const string &orig ) ;
public:
    				FONcTransform( DDS *dds,
					       BESDataHandlerInterface &dhi,
					       const string &localfile ) ;
    virtual			~FONcTransform() ;
    virtual int			transform( ) ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // FONcGridTransfrom_h_

