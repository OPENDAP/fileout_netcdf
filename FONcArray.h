// FONcArray.h

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#ifndef FONcArray_h_
#define FONcArray_h_ 1

#include <Array.h>

using namespace libdap;

#include "FONcBaseType.h"
#include "FONcDim.h"

class FONcMap;

/** @brief A DAP Array with file out netcdf information included
 *
 * This class represents a DAP Array with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP Array being converted
 */
class FONcArray: public FONcBaseType {
private:
    // The array being converted
    Array * _a;
    // The type of data stored in the array
    nc_type _array_type;
    // The number of dimensions to be stored in netcdf (if string, 2)
    int _ndims;
    // The actual number of dimensions of this array (if string, 1)
    int _actual_ndims;
    // The number of elements that will be stored in netcdf
    int _nelements;
    // The FONcDim dimensions to be used for this variable
    vector<FONcDim *> _dims;

    // Make these vector<> types. jhrg 10/12/15
    //
    // The netcdf dimension ids for this array
    int * _dim_ids;
    // The netcdf dimension sizes to be written
    size_t * _dim_sizes; // changed int to size_t. jhrg 12.27.2011
    // If string data, we need to do some comparison, so instead of
    // reading it more than once, read it once and save here
    string * _str_data;
    // If the array is already a map in a grid, then we don't want to
    // define it or write it.

    bool _dont_use_it;

    // Make this a vector<> jhrg 10/12/15
    // The netcdf chunk sizes for each dimension of this array.
    vector<size_t> d_chunksizes;

    // This is vector holds instances pf FONcMap* that wrap existing Array
    // objects that are pushed onto the global FONcGrid::Maps vector. Those
    // are never freed; I think the general pattern is to use the reference
    // counting pointers with FONcGrid::Maps. jhrg 8/28/13
    vector<FONcMap*> _grid_maps;

    FONcDim * find_dim(vector<string> &embed, const string &name, int size, bool ignore_size = false);
public:
    FONcArray(BaseType *b);
    virtual ~FONcArray();

    virtual void convert(vector<string> embed);
    virtual void define(int ncid);
    virtual void write(int ncid);

    virtual string name();
    virtual Array * array()
    {
        return _a;
    }

    virtual void dump(ostream &strm) const;

    static vector<FONcDim *> Dimensions;
};

#endif // FONcArray_h_

