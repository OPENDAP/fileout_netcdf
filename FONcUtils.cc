// FONcUtils.cc

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
#include "FONcUtils.h"

/** @brief convert the provided string to a netcdf allowed
 * identifier.
 *
 * The function makes a copy of the incoming parameter to use and
 * returns the new string.
 *
 * @param in identifier to convert
 * @returns new netcdf compliant identifier
 */
string
FONcUtils::id2netcdf( string in )
{
    // string of allowed characters in netcdf naming convention
    string allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+_.@" ;
    // string of allowed first characters in netcdf naming
    // convention
    string first = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" ;

    string::size_type i = 0;

    while( (i = in.find_first_not_of( allowed, i ) ) != string::npos)
    {
        in.replace( i, 1, "_" ) ;
        i++ ;
    }

    if( first.find( in[0] ) == string::npos )
    {
	in = (string)"h4_" + in ;
    }

    return in;
}

