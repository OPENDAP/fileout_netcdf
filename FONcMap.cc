#include <BESDebug.h>

#include "FONcMap.h"
#include "FONcUtils.h"

FONcMap::~FONcMap()
{
    if( _ingrid )
    {
	string name = _arr->name() ;
	delete _arr ;
	_arr = 0 ;
    }
}

void
FONcMap::decref()
{
    _ref-- ;
    if( !_ref ) delete this ;
}

/** @brief a method to compare two grid maps, or possible grid maps.
 *
 * All arrays are saved as an FONcMap if the array has only one dimension
 * and the name of the array and the name of the dimension are the same. The
 * maps are the same if their names are the same, they have the same number
 * of dimensions (arrays of strings written out have 2 dimensions, one for
 * the max length of the string), the type of the maps are the same, the
 * dimension size is the same, the dimension names are the same, and the
 * values of the maps are the same.
 *
 * @param tomap compare the saved map to this provided map
 * @return true if they are the same (shared) or false otherwise
 */
bool
FONcMap::compare( Array *tomap )
{
    bool isequal = true ;

    Array *map = _arr->array() ;

    BESDEBUG( "fonc", "FONcMap::compare - comparing " << tomap->name()
	      << " to " << map->name() << endl ) ;

    // compare the name
    if( isequal && tomap->name() != map->name() )
    {
	isequal = false ;
    }

    // compare the type
    if( isequal && tomap->var()->type() != map->var()->type() )
    {
	isequal =false ;
    }

    // compare the length of the array
    if( isequal && tomap->length() != map->length() )
    {
	isequal = false ;
    }

    // compare the number of dimensions
    if( isequal && tomap->dimensions() != map->dimensions() )
    {
	isequal = false ;
    }

    // the variable name needs to be the same as the dimension name
    if( isequal &&
        map->dimension_name( map->dim_begin() ) != map->name() )
    {
	isequal = false ;
    }

    // compare the dimension name
    if( isequal &&
        tomap->dimension_name( tomap->dim_begin() ) !=
	map->dimension_name( map->dim_begin() ) )
    {
	isequal = false ;
    }

    // compare the dimension size. Is this the same as the length of the array
    if( isequal &&
        tomap->dimension_size( tomap->dim_begin(), true ) !=
	map->dimension_size( map->dim_begin(), true ) )
    {
	isequal = false ;
    }

    if( isequal )
    {
	// compare the values of the array
	switch( tomap->var()->type() )
	{
	    case dods_byte_c:
		{
		    dods_byte my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_byte to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_int16_c:
		{
		    dods_int16 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_int16 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_uint16_c:
		{
		    dods_uint16 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_uint16 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_int32_c:
		{
		    dods_int32 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_int32 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_uint32_c:
		{
		    dods_uint32 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_uint32 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_float32_c:
		{
		    dods_float32 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_float32 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_float64_c:
		{
		    dods_float64 my_values[map->length()] ;
		    map->value( my_values ) ;
		    dods_float64 to_values[map->length()] ;
		    tomap->value( to_values ) ;
		    for( int i = 0; i < map->length(); i++ )
		    {
			if( my_values[i] != to_values[i] )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	    case dods_str_c:
	    case dods_url_c:
		{
		    vector<string> my_values ;
		    map->value( my_values ) ;
		    vector<string> to_values ;
		    tomap->value( to_values ) ;
		    vector<string>::const_iterator mi = my_values.begin() ;
		    vector<string>::const_iterator me = my_values.end() ;
		    vector<string>::const_iterator ti = to_values.begin() ;
		    for( ; mi != me; mi++, ti++ )
		    {
			if( (*mi) != (*ti) )
			{
			    isequal =  false ;
			    break ;
			}
		    }
		}
		break ;
	}
    }

    BESDEBUG( "fonc", "FONcMap::compare - done comparing " << tomap->name()
	      << " to " << map->name() << ": " << isequal << endl ) ;
    return isequal ;
}

void
FONcMap::add_grid( const string &name )
{
    _shared_by.push_back( name ) ;
}

void
FONcMap::clear_embedded()
{
    _arr->clear_embedded() ;
}

void
FONcMap::define( int ncid )
{
    if( !_defined )
    {
	_arr->define( ncid ) ;
	_defined = true ;
    }
}

void
FONcMap::write( int ncid )
{
    _arr->write( ncid ) ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcMap::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcMap::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "array:" ;
    if( _arr )
    {
	strm << endl ;
	BESIndent::Indent() ;
	_arr->dump( strm ) ;
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << " not set" << endl ;
    }
    strm << BESIndent::LMarg << "shared by: " ;
    vector<string>::const_iterator i = _shared_by.begin() ;
    vector<string>::const_iterator e = _shared_by.end() ;
    bool first = true ;
    for( ; i != e; i++ )
    {
	if( !first ) strm << ", " ;
	strm << (*i) ;
	first = false ;
    }
    strm << endl ;
    BESIndent::UnIndent() ;
}

