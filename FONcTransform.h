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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>

using std::string ;

#include <DDS.h>
#include <BaseType.h>

using namespace::libdap ;

class FONcTransform
{
private:
    static nc_type		get_nc_type( BaseType *element ) ;
    static void			write_structure( BaseType* b, int ncid ) ;
    static void			write_grid( BaseType* b, int ncid ) ;
    static void			write_array( BaseType* b, int ncid ) ;
    static void			write_var( BaseType* b, int ncid ) ;

public:
    static int			create_local_nc( DDS *dds, char* localfile ) ;
    static int			copy_all_attributes( const string &var_name,
    						     const string &source_file,
						     const string &target_file);
} ;

#endif // FONcGridTransfrom_h_
 