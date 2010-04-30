#include <BESInternalError.h>
#include <BESDebug.h>
#include <Int16.h>
#include <UInt16.h>

#include "FONcShort.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

FONcShort::FONcShort( BaseType *b )
    : FONcBaseType(), _bt( b )
{
    Int16 *i16 = dynamic_cast<Int16 *>(b) ;
    UInt16 *u16 = dynamic_cast<UInt16 *>(b) ;
    if( !i16 && !u16 )
    {
	string s = (string)"File out netcdf, FONcShort was passed a "
		   + "variable that is not a DAP Int16 or UInt16" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
}

FONcShort::~FONcShort()
{
}

void
FONcShort::define( int ncid )
{
    FONcBaseType::define( ncid ) ;

    if( !_defined )
    {
	FONcAttributes::add_attributes( ncid, _varid, _bt ) ;
	FONcAttributes::add_original_name( ncid, _varid,
					   _varname, _orig_varname ) ;

	_defined = true ;
    }
}

void
FONcShort::write( int ncid )
{
    BESDEBUG( "fonc", "FONcShort::write for var " << _varname << endl ) ;
    size_t var_index[] = {0} ;
    short *data = new short ;
    _bt->buf2val( (void**)&data ) ;
    int stax = nc_put_var1_short( ncid, _varid, var_index, data ) ;
    if( stax != NC_NOERR )
    {
	string err = (string)"fileout.netcdf - "
		     + "Failed to write short data for "
		     + _varname ;
	FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
    }
    delete data ;
    BESDEBUG( "fonc", "FONcShort::done write for var " << _varname << endl ) ;
}

string
FONcShort::name()
{
    return _bt->name() ;
}

nc_type
FONcShort::type()
{
    return NC_SHORT ;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcShort::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "FONcShort::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _bt->name()  << endl ;
    BESIndent::UnIndent() ;
}

