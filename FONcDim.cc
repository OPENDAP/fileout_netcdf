#include <sstream>

using std::ostringstream ;

#include <netcdf.h>

#include "FONcDim.h"
#include "FONcUtils.h"

int FONcDim::DimNameNum = 0 ;

FONcDim::FONcDim( const string &name, int size )
    : _name( name ), _size( size ), _dimid( 0 ), _defined( false ), _ref( 1 )
{
}

void
FONcDim::decref()
{
    _ref-- ;
    if( !_ref ) delete this ;
}

void
FONcDim::define( int ncid )
{
    if( !_defined )
    {
	if( _name.empty() )
	{
	    ostringstream dimname_strm ;
	    dimname_strm << "dim" << FONcDim::DimNameNum+1 ;
	    FONcDim::DimNameNum++ ;
	    _name = dimname_strm.str() ;
	}
	else
	{
	    _name = FONcUtils::id2netcdf( _name ) ;
	}
	int stax = nc_def_dim( ncid, _name.c_str(), _size, &_dimid ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"fileout.netcdf - "
			 + "Failed to add dimension " + _name ;
	    FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
	}
	_defined = true ;
    }
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcDim::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcDim::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _name << endl ;
    strm << BESIndent::LMarg << "size = " << _size << endl ;
    strm << BESIndent::LMarg << "dimid = " << _dimid << endl ;
    strm << BESIndent::LMarg << "already defined? " ;
    if( _defined )
	strm << "true" ;
    else
	strm << "false" ;
    strm << endl ;
    BESIndent::UnIndent() ;
}

