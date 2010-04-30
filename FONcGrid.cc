
#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcGrid.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

vector<FONcMap *> FONcGrid::Maps ;
bool FONcGrid::InGrid = false ;

FONcGrid::FONcGrid( BaseType *b )
    : FONcBaseType(), _grid( 0 ), _arr( 0 )
{
    _grid = dynamic_cast<Grid *>(b) ;
    if( !_grid )
    {
	string s = (string)"File out netcdf, FONcGrid was passed a "
		   + "variable that is not a DAP Grid" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

FONcGrid::~FONcGrid()
{
    bool done = false ;
    while( !done )
    {
	vector<FONcMap *>::iterator i = _maps.begin() ;
	vector<FONcMap *>::iterator e = _maps.end() ;
	if( i == e )
	{
	    done = true ;
	}
	else
	{
	    // These are the FONc types, not the actual ones
	    FONcMap *m = (*i) ;
	    m->decref() ;
	    _maps.erase( i ) ;
	}
    }
}

void
FONcGrid::convert( vector<string> embed )
{
    FONcGrid::InGrid = true ;
    FONcBaseType::convert( embed ) ;
    _varname = FONcUtils::gen_name( embed, _varname, _orig_varname ) ;
    BESDEBUG( "fonc", "FONcGrid::convert - converting grid "
                      << _varname << endl ) ;

    // A grid has maps, which are single dimnension arrays, and an array
    // with that many maps for dimensions.
    Grid::Map_iter mi = _grid->map_begin() ;
    Grid::Map_iter me = _grid->map_end() ;
    for( ; mi != me; mi++ )
    {
	Array *map = dynamic_cast<Array *>( (*mi) ) ;
	if( !map )
	{
	    string err = (string)"file out netcdf, grid "
			 + _varname + " map is not an array" ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}

	vector<string> map_embed ;

	vector<FONcMap *>::iterator vi = FONcGrid::Maps.begin() ;
	vector<FONcMap *>::iterator ve = FONcGrid::Maps.end() ;
	FONcMap *map_found = 0 ;
	bool done = false ;
	for( ; vi != ve && !done; vi++ )
	{
	    map_found = (*vi) ;
	    if( !map_found )
	    {
		throw BESInternalError("map_found is null.", __FILE__, __LINE__);
	    }
	    done = map_found->compare( map ) ;
	}
	// if we didn't find a match then done is still false. Add the
	// map to the vector of maps. If they are the same then create a
	// new FONcMap, add the grid name to the shared list and add the
	// FONcMap to the FONcGrid.
	if( !done )
	{
	    FONcArray *fa = new FONcArray( map ) ;
	    fa->convert( map_embed ) ;
	    map_found = new FONcMap( fa, true );
	    FONcGrid::Maps.push_back( map_found ) ;
	}
	else
	{
	    // it's the same ... we are sharing. Add the grid name fo
	    // the list of grids sharing this map and set the embedded
	    // name to empty, just using the name of the map.
	    map_found->incref() ;
	    map_found->add_grid( _varname ) ;
	    map_found->clear_embedded() ;
	}
	_maps.push_back( map_found ) ;
    }
    _arr = new FONcArray( _grid->get_array() ) ;
    _arr->convert( _embed ) ;

    BESDEBUG( "fonc", "FONcGrid::convert - done converting grid "
                      << _varname << endl ) ;
    FONcGrid::InGrid = false ;
}

void
FONcGrid::define( int ncid )
{
    if( !_defined )
    {
	BESDEBUG( "fonc", "FOncGrid::define - defining grid "
			  << _varname << endl ) ;

	vector<FONcMap *>::iterator i = _maps.begin() ;
	vector<FONcMap *>::iterator e = _maps.end() ;
	for( ; i != e; i++ )
	{
	    (*i)->define( ncid ) ;
	}
	_arr->define( ncid ) ;

	_defined = true ;

	BESDEBUG( "fonc", "FOncGrid::define - done defining grid "
			  << _varname << endl ) ;
    }
}

void
FONcGrid::write( int ncid )
{
    BESDEBUG( "fonc", "FOncGrid::define - writing grid "
		      << _varname << endl ) ;

    vector<FONcMap *>::iterator i = _maps.begin() ;
    vector<FONcMap *>::iterator e = _maps.end() ;
    for( ; i != e; i++ )
    {
	(*i)->write( ncid ) ;
    }
    _arr->write( ncid ) ;

    _defined = true ;

    BESDEBUG( "fonc", "FOncGrid::define - done writing grid "
		      << _varname << endl ) ;
}

string
FONcGrid::name()
{
    return _grid->name() ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcGrid::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcGrid::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _grid->name() << " { " << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "maps:" ;
    if( _maps.size() )
    {
	strm << endl ;
	BESIndent::Indent() ;
	vector<FONcMap *>::const_iterator i = _maps.begin() ;
	vector<FONcMap *>::const_iterator e = _maps.end() ;
	for( ; i != e; i++ )
	{
	    FONcMap *m = (*i) ;
	    m->dump( strm ) ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << " empty" << endl ;
    }
    BESIndent::UnIndent() ;
    strm << BESIndent::LMarg << "}" << endl ;
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
    BESIndent::UnIndent() ;
}

