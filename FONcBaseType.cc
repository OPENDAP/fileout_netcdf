#include <BESDebug.h>

#include "FONcBaseType.h"
#include "FONcUtils.h"

void
FONcBaseType::convert( vector<string> embed )
{
    _embed = embed ;
    _varname = name() ;
}

/** @brief Define the variable in the netcdf file
 *
 * This method creates this variable in the netcdf file. This
 * implementation is used for only simple types (byte, short, int,
 * float, double), and not for the complex types (str, structure, array,
 * grid, sequence)
 *
 * @param ncid Id of the NetCDF file
 * @throws BESInternalError if defining the variable fails
 */
void
FONcBaseType::define( int ncid )
{
    if( !_defined )
    {
	_varname = FONcUtils::gen_name( _embed, _varname, _orig_varname ) ;
	BESDEBUG( "fonc", "FONcBaseType::define - defining "
			  << _varname << endl ) ;
	int stax = nc_def_var( ncid, _varname.c_str(), type(),
			       0, NULL, &_varid ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"fileout.netcdf - "
			 + "Failed to define variable "
			 + _varname ;
	    FONcUtils::handle_error( stax, err, __FILE__, __LINE__ ) ;
	}

	BESDEBUG( "fonc", "FONcBaseType::define - done defining "
			  << _varname << endl ) ;
    }
}

/** @brief Returns the type of data of this variable
 *
 * This implementation of the method returns the default type of data.
 * Subclasses of FONcBaseType will return the specific type of data for
 * simple types
 */
nc_type
FONcBaseType::type()
{
    return NC_NAT ; // the constant ncdf uses to define simple type
}

void
FONcBaseType::clear_embedded()
{
    _embed.clear() ;
}

