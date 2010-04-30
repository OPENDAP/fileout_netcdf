
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

