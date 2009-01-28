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
 
// (c) COPYRIGHT University Corporation for Atmostpheric Research 2004-2005
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

using std::string ;
using std::vector ;

#include <DDS.h>

using namespace::libdap ;

#include <BESObj.h>

class FONcTransform : public BESObj
{
private:
    int				_ncid ;
    DDS				*_dds ;
    string			_localfile ;
    vector<BaseType *>		_embedded ;

    nc_type			get_nc_type( BaseType *element ) ;
    void			write_structure( BaseType* b ) ;
    void			write_grid( BaseType* b ) ;
    void			write_array( BaseType* b ) ;
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

    void			handle_error( int stax, string &err,
					      const string &file, int line ) ;
public:
    				FONcTransform( DDS *dds,
					       const string &localfile ) ;
    virtual			~FONcTransform() ;
    virtual int			transform( ) ;

    virtual void		dump( ostream &strm ) const ;
} ;

#endif // FONcGridTransfrom_h_
 
