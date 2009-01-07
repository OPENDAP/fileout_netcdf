// arrayT.cc

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <netcdf.h>

#include <fstream>
#include <iostream>
#include <sstream>

using std::ofstream ;
using std::ios ;
using std::cerr ;
using std::endl ;
using std::ostringstream ;

#include <DataDDS.h>
#include <Array.h>
#include <Byte.h>
#include <Int16.h>
#include <Int32.h>
#include <UInt16.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>

using namespace::libdap ;

#include <BESDataDDSResponse.h>
#include <BESDataHandlerInterface.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include "test_config.h"
#include "FONcTransmitter.h"

class arrayT: public TestFixture {
private:
public:
    arrayT() {}
    ~arrayT() {}

    void setUp()
    {
	string bes_conf = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
	putenv( (char *)bes_conf.c_str() ) ;
	BESDebug::SetUp( "cerr,fonc" ) ;
    }

    void tearDown()
    {
	// remove the temporary file that we created
	/*
	if( !access( "arrayT.nc", F_OK ) )
	{
	    remove( "arrayT.nc" ) ;
	}
	*/
    }

    CPPUNIT_TEST_SUITE( arrayT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void handle_error( int stax, string &err )
    {
	if( stax != NC_NOERR )
	{
	    const char *nerr = nc_strerror( stax ) ;
	    if( nerr )
	    {
		err += (string)": " + nerr ;
	    }
	    else
	    {
		err += (string)": unknown error" ;
	    }
	    cerr << err << endl ;

	    CPPUNIT_ASSERT( false ) ;
	}
    }

    void check_var( int ncid, const string &name )
    {
	int varid = 0 ;
	int stax = nc_inq_varid( ncid, name.c_str(), &varid ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"Failed to check variable " + name ;
	    handle_error( stax, err ) ;
	}

	char varname[NC_MAX_NAME+1] ;
	nc_type type ;
	int ndims = 0 ;
	int dims[NC_MAX_VAR_DIMS] ;
	int natts = 0 ;
	stax = nc_inq_var( ncid, varid, varname, &type, &ndims, dims, &natts ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"Failed to inquire variable " + name ;
	    handle_error( stax, err ) ;
	}
	int len = strlen( name.c_str() ) ;
	cerr << "name: " << name
	     << ", varname: " << varname << endl ;
	CPPUNIT_ASSERT( name == varname ) ;
	size_t index[] = {0} ;
	if( name == "byte_array" )
	{
	    // 2 dims, sizes 2 and 5
	    CPPUNIT_ASSERT( ndims == 2 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    size_t dimsize = 0 ;
	    int dim = 0 ;
	    size_t dim_sizes[] = {2,5} ;
	    int nelements = 10 ;
	    for( dim = 0; dim < ndims; dim++ )
	    {
		stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to inquire dims" ;
		    handle_error( stax, err ) ;
		}
		ostringstream baseline ; 
		baseline << "byte_dim" << dim+1 ;
		CPPUNIT_ASSERT( baseline.str() == dimname ) ;
		CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;

		unsigned char *data = new unsigned char[nelements] ;
		stax = nc_get_var_uchar( ncid, varid, data ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to get byte data" ;
		    handle_error( stax, err ) ;
		}
		for( int element = 0; element < nelements; element++ )
		{
		    CPPUNIT_ASSERT( data[element] == element ) ;
		}
	    }
	}
	else if( name == "i16_array" )
	{
	    // 2 dims, sizes 3 and 6
	    CPPUNIT_ASSERT( ndims == 2 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    size_t dimsize = 0 ;
	    int dim = 0 ;
	    size_t dim_sizes[] = {3,6} ;
	    int nelements = 18 ;
	    for( dim = 0; dim < ndims; dim++ )
	    {
		stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to inquire dims" ;
		    handle_error( stax, err ) ;
		}
		ostringstream baseline ; 
		baseline << "i16_dim" << dim+1 ;
		CPPUNIT_ASSERT( baseline.str() == dimname ) ;
		CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;
	    }
	    short *data = new short [nelements] ;
	    stax = nc_get_var_short( ncid, varid, data ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get i16 data" ;
		handle_error( stax, err ) ;
	    }
	    for( short element = 0; element < nelements; element++ )
	    {
		CPPUNIT_ASSERT( data[element] == element*(-16) ) ;
	    }
	}
	else if( name == "i32_array" )
	{
	    // 2 dims, sizes 2 and 20
	    CPPUNIT_ASSERT( ndims == 2 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    size_t dimsize = 0 ;
	    int dim = 0 ;
	    size_t dim_sizes[] = {2,20} ;
	    int nelements = 40 ;
	    for( dim = 0; dim < ndims; dim++ )
	    {
		stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to inquire dims" ;
		    handle_error( stax, err ) ;
		}
		ostringstream baseline ; 
		baseline << "i32_dim" << dim+1 ;
		CPPUNIT_ASSERT( baseline.str() == dimname ) ;
		CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;
	    }
	    int *data = new int[nelements] ;
	    stax = nc_get_var_int( ncid, varid, data ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get i32 data" ;
		handle_error( stax, err ) ;
	    }
	    for( int element = 0; element < nelements; element++ )
	    {
		CPPUNIT_ASSERT( data[element] == element*(-512) ) ;
	    }
	}
	else if( name == "ui16_array" )
	{
	    // 2 dims, sizes 3 and 7
	    CPPUNIT_ASSERT( ndims == 2 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    size_t dimsize = 0 ;
	    int dim = 0 ;
	    size_t dim_sizes[] = {3,7} ;
	    int nelements = 21 ;
	    for( dim = 0; dim < ndims; dim++ )
	    {
		stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to inquire dims" ;
		    handle_error( stax, err ) ;
		}
		ostringstream baseline ; 
		baseline << "ui16_dim" << dim+1 ;
		CPPUNIT_ASSERT( baseline.str() == dimname ) ;
		CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;
	    }
	    short *data = new short [nelements] ;
	    stax = nc_get_var_short( ncid, varid, data ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get ui16 data" ;
		handle_error( stax, err ) ;
	    }
	    for( short element = 0; element < nelements; element++ )
	    {
		CPPUNIT_ASSERT( data[element] == element*16 ) ;
	    }
	}
	else if( name == "ui32_array" )
	{
	    // 2 dims, sizes 3 and 40
	    CPPUNIT_ASSERT( ndims == 2 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    size_t dimsize = 0 ;
	    int dim = 0 ;
	    size_t dim_sizes[] = {3,40} ;
	    int nelements = 120 ;
	    for( dim = 0; dim < ndims; dim++ )
	    {
		stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to inquire dims" ;
		    handle_error( stax, err ) ;
		}
		ostringstream baseline ; 
		baseline << "ui32_dim" << dim+1 ;
		CPPUNIT_ASSERT( baseline.str() == dimname ) ;
		CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;
	    }
	    int *data = new int[nelements] ;
	    stax = nc_get_var_int( ncid, varid, data ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get ui32 data" ;
		handle_error( stax, err ) ;
	    }
	    for( int element = 0; element < nelements; element++ )
	    {
		CPPUNIT_ASSERT( data[element] == element*512 ) ;
	    }
	}
	else if( name == "f32_array" )
	{
	    // 1 dims, size 100
	    CPPUNIT_ASSERT( ndims == 1 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    size_t dimsize = 0 ;
	    int dim = 0 ;
	    size_t dim_sizes[] = {100} ;
	    int nelements = 100 ;
	    for( dim = 0; dim < ndims; dim++ )
	    {
		stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to inquire dims" ;
		    handle_error( stax, err ) ;
		}
		ostringstream baseline ; 
		baseline << "f32_dim" << dim+1 ;
		CPPUNIT_ASSERT( baseline.str() == dimname ) ;
		CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;
	    }
	    float *data = new float[nelements] ;
	    stax = nc_get_var_float( ncid, varid, data ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get f32 data" ;
		handle_error( stax, err ) ;
	    }
	    for( int element = 0; element < nelements; element++ )
	    {
		float baseline = element*5.7862 ;
		CPPUNIT_ASSERT( data[element] == baseline ) ;
	    }
	}
	else if( name == "f64_array" )
	{
	    // 3 dims, size 2,2,100
	    CPPUNIT_ASSERT( ndims == 3 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    size_t dimsize = 0 ;
	    int dim = 0 ;
	    size_t dim_sizes[] = {2,2,100} ;
	    int nelements = 400 ;
	    for( dim = 0; dim < ndims; dim++ )
	    {
		stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to inquire dims" ;
		    handle_error( stax, err ) ;
		}
		ostringstream baseline ; 
		baseline << "f64_dim" << dim+1 ;
		CPPUNIT_ASSERT( baseline.str() == dimname ) ;
		CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;
	    }
	    double *data = new double[nelements] ;
	    stax = nc_get_var_double( ncid, varid, data ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get f64 data" ;
		handle_error( stax, err ) ;
	    }
	    for( int element = 0; element < nelements; element++ )
	    {
		double baseline = element*589.288 ;
		CPPUNIT_ASSERT( data[element] == baseline ) ;
	    }
	}
	else if( name == "str_array" )
	{
	    // 4 dims, size 2,3,6,10
	    CPPUNIT_ASSERT( ndims == 4 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    size_t dimsize = 0 ;
	    int dim = 0 ;
	    size_t dim_sizes[] = {2,3,6,10} ;
	    for( dim = 0; dim < ndims-1; dim++ )
	    {
		stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
		if( stax != NC_NOERR )
		{
		    string err = (string)"Failed to inquire dims" ;
		    handle_error( stax, err ) ;
		}
		ostringstream baseline ; 
		baseline << "str_dim" << dim+1 ;
		CPPUNIT_ASSERT( baseline.str() == dimname ) ;
		CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;
	    }
	    stax = nc_inq_dim( ncid, dims[dim], dimname, &dimsize ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to inquire dims" ;
		handle_error( stax, err ) ;
	    }
	    string basename = "str_array_len" ;
	    CPPUNIT_ASSERT( basename == dimname ) ;
	    CPPUNIT_ASSERT( dimsize == dim_sizes[dim] ) ;
	    size_t var_count[4] ;
	    size_t var_start[4] ;
	    for( int i = 0; i < 2; i++ )
	    {
		for( int j = 0; j < 3; j++ )
		{
		    for( int k = 0; k < 6; k++ )
		    {
			var_count[0] = 1 ;
			var_count[1] = 1 ;
			var_count[2] = 1 ;
			var_count[3] = 10 ;
			var_start[0] = i ;
			var_start[1] = j ;
			var_start[2] = k ;
			var_start[3] = 0 ;
			char data[11] ;
			stax = nc_get_vara_text( ncid, varid, var_start,
						 var_count, data ) ;
			ostringstream baseline ;
			baseline << "str_" << i << "." << j << "." << k ;
			CPPUNIT_ASSERT( baseline.str() == data ) ;
		    }
		}
	    }
	}
	else
	{
	    CPPUNIT_ASSERT( !"Unknown variable to check" ) ;
	}
    }

    void do_test()
    {
	try
	{
	    // build a DataDDS of simple types and set values for each of the
	    // simple types.
	    DataDDS *dds = new DataDDS( NULL, "virtual" ) ;
	    {
		Byte *b = new Byte( "byte_array" ) ;
		Array *a = new Array( "array", b ) ;
		delete b ; b = 0 ;
		a->append_dim( 2, "byte_dim1" ) ;
		a->append_dim( 5, "byte_dim2" ) ;
		dds->add_var( a ) ;
		delete a ; a = 0 ;

		a = dynamic_cast<Array *>(dds->var( "byte_array" ) ) ;
		CPPUNIT_ASSERT( a ) ;

		vector<dods_byte> ba ;
		for( dods_byte i = 0; i < 10; i++ )
		{
		    ba.push_back( i ) ;
		}
		a->set_value( ba, ba.size() ) ;
	    }
	    {
		Int16 *i16 = new Int16( "i16_array" ) ;
		Array *a = new Array( "array", i16 ) ;
		delete i16 ; i16 = 0 ;
		a->append_dim( 3, "i16_dim1" ) ;
		a->append_dim( 6, "i16_dim2" ) ;
		dds->add_var( a ) ;
		delete a ; a = 0 ;

		a = dynamic_cast<Array *>(dds->var( "i16_array" ) ) ;
		CPPUNIT_ASSERT( a ) ;

		vector<dods_int16> i16a ;
		for( dods_int16 i = 0; i < 18; i++ )
		{
		    i16a.push_back( i*(-16) ) ;
		}
		a->set_value( i16a, i16a.size() ) ;
	    }
	    {
		Int32 *i32 = new Int32( "i32_array" ) ;
		Array *a = new Array( "array", i32 ) ;
		delete i32 ; i32 = 0 ;
		a->append_dim( 2, "i32_dim1" ) ;
		a->append_dim( 20, "i32_dim2" ) ;
		dds->add_var( a ) ;
		delete a ; a = 0 ;

		a = dynamic_cast<Array *>(dds->var( "i32_array" ) ) ;
		CPPUNIT_ASSERT( a ) ;

		vector<dods_int32> i32a ;
		for( dods_int32 i = 0; i < 40; i++ )
		{
		    i32a.push_back( i*(-512) ) ;
		}
		a->set_value( i32a, i32a.size() ) ;
	    }
	    {
		UInt16 *ui16 = new UInt16( "ui16_array" ) ;
		Array *a = new Array( "array", ui16 ) ;
		delete ui16 ; ui16 = 0 ;
		a->append_dim( 3, "ui16_dim1" ) ;
		a->append_dim( 7, "ui16_dim2" ) ;
		dds->add_var( a ) ;
		delete a ; a = 0 ;

		a = dynamic_cast<Array *>(dds->var( "ui16_array" )) ;
		CPPUNIT_ASSERT( a ) ;

		vector<dods_uint16> ui16a ;
		for( dods_uint16 i = 0; i < 21; i++ )
		{
		    ui16a.push_back( i*16 ) ;
		}
		a->set_value( ui16a, ui16a.size() ) ;
	    }
	    {
		UInt32 *ui32 = new UInt32( "ui32_array" ) ;
		Array *a = new Array( "array", ui32 ) ;
		delete ui32 ; ui32 = 0 ;
		a->append_dim( 3, "ui32_dim1" ) ;
		a->append_dim( 40, "ui32_dim2" ) ;
		dds->add_var( a ) ;
		delete a ; a = 0 ;

		a = dynamic_cast<Array *>(dds->var( "ui32_array" ) ) ;
		CPPUNIT_ASSERT( a ) ;

		vector<dods_uint32> ui32a ;
		for( dods_uint32 i = 0; i < 120; i++ )
		{
		    ui32a.push_back( i*512 ) ;
		}
		a->set_value( ui32a, ui32a.size() ) ;
	    }
	    {
		Float32 *f32 = new Float32( "f32_array" ) ;
		Array *a = new Array( "array", f32 ) ;
		delete f32 ; f32 = 0 ;
		a->append_dim( 100, "f32_dim1" ) ;
		dds->add_var( a ) ;
		delete a ; a = 0 ;

		a = dynamic_cast<Array *>(dds->var( "f32_array" ) ) ;
		CPPUNIT_ASSERT( a ) ;

		vector<dods_float32> f32a ;
		for( int i = 0; i < 100; i++ )
		{
		    f32a.push_back( i*5.7862 ) ;
		}
		a->set_value( f32a, f32a.size() ) ;
	    }
	    {
		Float64 *f64 = new Float64( "f64_array" ) ;
		Array *a = new Array( "array", f64 ) ;
		delete f64 ; f64 = 0 ;
		a->append_dim( 2, "f64_dim1" ) ;
		a->append_dim( 2, "f64_dim2" ) ;
		a->append_dim( 100, "f64_dim3" ) ;
		dds->add_var( a ) ;
		delete a ; a = 0 ;

		a = dynamic_cast<Array *>(dds->var( "f64_array" ) ) ;
		CPPUNIT_ASSERT( a ) ;

		vector<dods_float64> f64a ;
		for( int i = 0; i < 400; i++ )
		{
		    f64a.push_back( i*589.288 ) ;
		}
		a->set_value( f64a, f64a.size() ) ;
	    }
	    {
		Str *str = new Str( "str_array" ) ;
		Array *a = new Array( "array", str ) ;
		delete str ; str = 0 ;
		a->append_dim( 2, "str_dim1" ) ;
		a->append_dim( 3, "str_dim2" ) ;
		a->append_dim( 6, "str_dim3" ) ;
		dds->add_var( a ) ;
		delete a ; a = 0 ;

		a = dynamic_cast<Array *>(dds->var( "str_array" ) ) ;
		CPPUNIT_ASSERT( a ) ;

		vector<string> stra ;
		for( int i = 0; i < 2; i++ )
		{
		    for( int j = 0; j < 3; j++ )
		    {
			for( int k = 0; k < 6; k++ )
			{
			    ostringstream strm ;
			    strm << "str_" << i << "." << j << "." << k ;
			    stra.push_back( strm.str() ) ;
			}
		    }
		}
		a->set_value( stra, stra.size() ) ;
	    }

	    // transform the DataDDS into a netcdf file. The dhi only needs the
	    // output stream and the post constraint. Test no constraints and
	    // then some different constraints (1 var, 2 var)

	    // The resulting netcdf file is streamed back. Write this file to a
	    // test file locally
	    BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	    BESDataHandlerInterface dhi ;
	    ofstream fstrm( "./arrayT.nc", ios::out|ios::trunc ) ;
	    dhi.set_output_stream( &fstrm ) ;
	    dhi.data[POST_CONSTRAINT] = "" ;
	    FONcTransmitter ft ;
	    FONcTransmitter::send_data( obj, dhi ) ;
	    fstrm.close() ;

	    // deleting the response object deletes the DataDDS
	    delete obj ;

	    // open the netcdf file and check the contents. NO ATTRIBUTES TO
	    // START WITH.
	    int ncid = 0 ;
	    int stax = nc_open( "./arrayT.nc", NC_NOWRITE, &ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to open arrayT.nc" ;
		handle_error( stax, err ) ;
	    }

	    int nvars = 0 ;
	    stax = nc_inq_nvars( ncid, &nvars ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire nvars" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( nvars == 8 ) ;

	    int ndims = 0 ;
	    stax = nc_inq_ndims( ncid, &ndims ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire ndims" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( ndims == 18 ) ;

	    check_var( ncid, "byte_array" ) ;
	    check_var( ncid, "i16_array" ) ;
	    check_var( ncid, "i32_array" ) ;
	    check_var( ncid, "ui16_array" ) ;
	    check_var( ncid, "ui32_array" ) ;
	    check_var( ncid, "f32_array" ) ;
	    check_var( ncid, "f64_array" ) ;
	    check_var( ncid, "str_array" ) ;

	    stax = nc_close( ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to close arrayT.nc" ;
		handle_error( stax, err ) ;
	    }

	    CPPUNIT_ASSERT( true ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( false ) ;
	}
    }
} ;

CPPUNIT_TEST_SUITE_REGISTRATION( arrayT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

