// FONcSequence.cc

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

#include "FONcSequence.h"
#include "FONcUtils.h"

FONcSequence::FONcSequence( BaseType *b )
    : FONcBaseType(), _s( 0 )
{
    _s = dynamic_cast<Sequence *>(b) ;
    if( !_s )
    {
	string s = (string)"File out netcdf, FONcSequence was passed a "
		   + "variable that is not a DAP Sequence" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

FONcSequence::~FONcSequence()
{
}

void
FONcSequence::convert( vector<string> embed )
{
    FONcBaseType::convert( embed ) ;
    _varname = FONcUtils::gen_name( embed, _varname, _orig_varname ) ;
}

void
FONcSequence::define( int ncid )
{
    // for now we are simply going to add a global variable noting the
    // presence of the sequence, the name of the sequence, and that the
    // sequences has been elided.
    string val = (string)"The sequence " + _varname
		 + " is a member of this dataset and has been elided." ;
    int stax = nc_put_att_text( ncid, NC_GLOBAL, _varname.c_str(),
				val.length(), val.c_str() ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"File out netcdf, "
		     + "failed to write string attribute for sequence "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
}

void
FONcSequence::write( int ncid )
{
}

string
FONcSequence::name()
{
    return _s->name() ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcSequence::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcSequence::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _s->name()  << endl ;
    BESIndent::UnIndent() ;
}

