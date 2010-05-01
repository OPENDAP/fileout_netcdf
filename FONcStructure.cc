// FONcStructure.cc

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

#include "FONcStructure.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

FONcStructure::FONcStructure( BaseType *b )
    : FONcBaseType(), _s( 0 )
{
    _s = dynamic_cast<Structure *>(b) ;
    if( !_s )
    {
	string s = (string)"File out netcdf, write_structure was passed a "
		   + "variable that is not a structure" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

FONcStructure::~FONcStructure()
{
    bool done = false ;
    while( !done )
    {
	vector<FONcBaseType *>::iterator i = _vars.begin() ;
	vector<FONcBaseType *>::iterator e = _vars.end() ;
	if( i == e )
	{
	    done = true ;
	}
	else
	{
	    // These are the FONc types, not the actual ones
	    FONcBaseType *b = (*i) ;
	    delete b ;
	    _vars.erase( i ) ;
	}
    }
}

void
FONcStructure::convert( vector<string> embed )
{
    FONcBaseType::convert( embed ) ;
    embed.push_back( name() ) ;
    Constructor::Vars_iter vi = _s->var_begin() ;
    Constructor::Vars_iter ve = _s->var_end() ;
    for( ; vi != ve; vi++ )
    {
	BaseType *bt = *vi ;
	if( bt->send_p() )
	{
	    BESDEBUG( "fonc", "FONcStructure::convert - converting "
			      << bt->name() << endl ) ;
	    FONcBaseType *fbt = FONcUtils::convert( bt ) ;
	    _vars.push_back( fbt ) ;
	    fbt->convert( embed ) ;
	}
    }
}

void
FONcStructure::define( int ncid )
{
    if( !_defined )
    {
	BESDEBUG( "fonc", "FONcStructure::define - defining "
			  << _varname << endl ) ;
	vector<FONcBaseType *>::const_iterator i = _vars.begin() ;
	vector<FONcBaseType *>::const_iterator e = _vars.end() ;
	for( ; i != e; i++ )
	{
	    FONcBaseType *fbt = (*i) ;
	    BESDEBUG( "fonc", "defining " << fbt->name() << endl ) ;
	    fbt->define( ncid ) ;
	}

	_defined = true ;

	BESDEBUG( "fonc", "FONcStructure::define - done defining "
			  << _varname << endl ) ;
    }
}

void
FONcStructure::write( int ncid )
{
    BESDEBUG( "fonc", "FONcStructure::write - writing "
		      << _varname << endl ) ;
    vector<FONcBaseType *>::const_iterator i = _vars.begin() ;
    vector<FONcBaseType *>::const_iterator e = _vars.end() ;
    for( ; i != e; i++ )
    {
	FONcBaseType *fbt = (*i) ;
	fbt->write( ncid ) ;
    }
    BESDEBUG( "fonc", "FONcStructure::define - done writing "
		      << _varname << endl ) ;
}

string
FONcStructure::name()
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
FONcStructure::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcStructure::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _s->name()  << " {" << endl ;
    BESIndent::Indent() ;
    vector<FONcBaseType *>::const_iterator i = _vars.begin() ;
    vector<FONcBaseType *>::const_iterator e = _vars.end() ;
    for( ; i != e; i++ )
    {
	FONcBaseType *fbt = *i ;
	fbt->dump( strm ) ;
    }
    BESIndent::UnIndent() ;
    strm << BESIndent::LMarg << "}" << endl ;
    BESIndent::UnIndent() ;
}

