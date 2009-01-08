// structT.cc

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <netcdf.h>

#include <fstream>
#include <iostream>

using std::ofstream ;
using std::ios ;
using std::cerr ;
using std::endl ;

#include <DataDDS.h>
#include <Structure.h>
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

class structT: public TestFixture {
private:
public:
    structT() {}
    ~structT() {}

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
	if( !access( "structT.nc", F_OK ) )
	{
	    remove( "structT.nc" ) ;
	}
	*/
    }

    CPPUNIT_TEST_SUITE( structT ) ;

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

    void check_var( int ncid, const string &embed, const string &name )
    {
	string fullname = embed + "." + name ;
	int varid = 0 ;
	int stax = nc_inq_varid( ncid, fullname.c_str(), &varid ) ;
	if( stax != NC_NOERR )
	{
	    string err = (string)"Failed to check variable " + fullname ;
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
	    string err = (string)"Failed to inquire variable " + fullname ;
	    handle_error( stax, err ) ;
	}
	int len = strlen( fullname.c_str() ) ;
	cerr << "fullname: " << fullname
	     << ", varname: " << varname << endl ;
	CPPUNIT_ASSERT( fullname == varname ) ;
	size_t index[] = {0} ;
	if( name == "byte" )
	{
	    CPPUNIT_ASSERT( ndims == 0 ) ;
	    unsigned char val = 0 ;
	    stax = nc_get_var1_uchar( ncid, varid, index, &val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get uchar for " + fullname ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( val == 28 ) ;
	}
	else if( name == "i16" )
	{
	    CPPUNIT_ASSERT( ndims == 0 ) ;
	    short val = 0 ;
	    stax = nc_get_var1_short( ncid, varid, index, &val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get short for " + fullname ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( val == -2048 ) ;
	}
	else if( name == "i32" )
	{
	    CPPUNIT_ASSERT( ndims == 0 ) ;
	    int val = 0 ;
	    stax = nc_get_var1_int( ncid, varid, index, &val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get int for " + fullname ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( val == -105467 ) ;
	}
	else if( name == "ui16" )
	{
	    CPPUNIT_ASSERT( ndims == 0 ) ;
	    short val = 0 ;
	    stax = nc_get_var1_short( ncid, varid, index, &val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get short for " + fullname ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( val == 2048 ) ;
	}
	else if( name == "ui32" )
	{
	    CPPUNIT_ASSERT( ndims == 0 ) ;
	    int val = 0 ;
	    stax = nc_get_var1_int( ncid, varid, index, &val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get int for " + fullname ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( val == 105467 ) ;
	}
	else if( name == "f32" )
	{
	    CPPUNIT_ASSERT( ndims == 0 ) ;
	    float val = 0.0 ;
	    stax = nc_get_var1_float( ncid, varid, index, &val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get float for " + fullname ;
		handle_error( stax, err ) ;
	    }
	    float baseline = 5.7866 ;
	    CPPUNIT_ASSERT( val == baseline ) ;
	}
	else if( name == "f64" )
	{
	    CPPUNIT_ASSERT( ndims == 0 ) ;
	    double val = 0.0 ;
	    stax = nc_get_var1_double( ncid, varid, index, &val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get double for " + fullname ;
		handle_error( stax, err ) ;
	    }
	    double baseline = 10245.1234 ;
	    CPPUNIT_ASSERT( val == baseline ) ;
	}
	else if( name == "str" )
	{
	    CPPUNIT_ASSERT( ndims == 1 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    string dnbaseline = embed + "." + "str_len" ;
	    size_t dimsize = 0 ;
	    stax = nc_inq_dim( ncid, dims[0], dimname, &dimsize ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to inquire dims" ;
		handle_error( stax, err ) ;
	    }
	    cerr << "dimname: " << dimname
	         << ", baseline: " << dnbaseline << endl ;
	    CPPUNIT_ASSERT( dnbaseline == dimname ) ;
	    CPPUNIT_ASSERT( dimsize == 23 ) ;

	    char val[dimsize+1] ;
	    size_t start[1] = {0} ;
	    size_t count[1] = {dimsize} ;
	    stax = nc_get_vara_text( ncid, varid, start, count, val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get str value" ;
		handle_error( stax, err ) ;
	    }
	    string baseline = "This is a String Value" ;
	    CPPUNIT_ASSERT( baseline == val ) ;
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

	    Structure s( "mystruct" ) ;

	    Byte b( "byte" ) ;
	    b.set_value( 28 ) ;
	    s.add_var( &b ) ;

	    Int16 i16( "i16" ) ;
	    i16.set_value( -2048 ) ;
	    s.add_var( &i16 ) ;

	    Int32 i32( "i32" ) ;
	    i32.set_value( -105467 ) ;
	    s.add_var( &i32 ) ;

	    UInt16 ui16( "ui16" ) ;
	    ui16.set_value( 2048 ) ;
	    s.add_var( &ui16 ) ;

	    UInt32 ui32( "ui32" ) ;
	    ui32.set_value( 105467 ) ;
	    s.add_var( &ui32 ) ;

	    Float32 f32( "f32" ) ;
	    f32.set_value( 5.7866 ) ;
	    s.add_var( &f32 ) ;

	    Float64 f64( "f64" ) ;
	    f64.set_value( 10245.1234 ) ;
	    s.add_var( &f64 ) ;

	    Str str( "str" ) ;
	    str.set_value( "This is a String Value" ) ;
	    s.add_var( &str ) ;

	    dds->add_var( &s ) ;

	    // transform the DataDDS into a netcdf file. The dhi only needs the
	    // output stream and the post constraint. Test no constraints and
	    // then some different constraints (1 var, 2 var)

	    // The resulting netcdf file is streamed back. Write this file to a
	    // test file locally
	    BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	    BESDataHandlerInterface dhi ;
	    ofstream fstrm( "./structT.nc", ios::out|ios::trunc ) ;
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
	    int stax = nc_open( "./structT.nc", NC_NOWRITE, &ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to open structT.nc" ;
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
	    CPPUNIT_ASSERT( ndims == 1 ) ;

	    check_var( ncid, "mystruct", "byte" ) ;
	    check_var( ncid, "mystruct", "i16" ) ;
	    check_var( ncid, "mystruct", "i32" ) ;
	    check_var( ncid, "mystruct", "ui16" ) ;
	    check_var( ncid, "mystruct", "ui32" ) ;
	    check_var( ncid, "mystruct", "f32" ) ;
	    check_var( ncid, "mystruct", "f64" ) ;
	    check_var( ncid, "mystruct", "str" ) ;

	    stax = nc_close( ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to close structT.nc" ;
		handle_error( stax, err ) ;
	    }

	    CPPUNIT_ASSERT( true ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( false ) ;
	}

	try
	{
	    // build a DataDDS of simple types and set values for each of the
	    // simple types.
	    DataDDS *dds = new DataDDS( NULL, "virtual" ) ;

	    Structure s1( "s1" ) ;
	    Structure s2( "s2" ) ;
	    Structure s3( "s3" ) ;

	    Byte b( "byte" ) ;
	    b.set_value( 28 ) ;
	    s1.add_var( &b ) ;

	    Int16 i16( "i16" ) ;
	    i16.set_value( -2048 ) ;
	    s2.add_var( &i16 ) ;

	    Int32 i32( "i32" ) ;
	    i32.set_value( -105467 ) ;
	    s3.add_var( &i32 ) ;

	    UInt16 ui16( "ui16" ) ;
	    ui16.set_value( 2048 ) ;
	    s1.add_var( &ui16 ) ;

	    UInt32 ui32( "ui32" ) ;
	    ui32.set_value( 105467 ) ;
	    s2.add_var( &ui32 ) ;

	    Float32 f32( "f32" ) ;
	    f32.set_value( 5.7866 ) ;
	    s3.add_var( &f32 ) ;

	    Float64 f64( "f64" ) ;
	    f64.set_value( 10245.1234 ) ;
	    s1.add_var( &f64 ) ;

	    Str str( "str" ) ;
	    str.set_value( "This is a String Value" ) ;
	    s2.add_var( &str ) ;

	    s2.add_var( &s3 ) ;
	    s1.add_var( &s2 ) ;

	    dds->add_var( &s1 ) ;

	    // transform the DataDDS into a netcdf file. The dhi only needs the
	    // output stream and the post constraint. Test no constraints and
	    // then some different constraints (1 var, 2 var)

	    // The resulting netcdf file is streamed back. Write this file to a
	    // test file locally
	    BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	    BESDataHandlerInterface dhi ;
	    ofstream fstrm( "./structT.nc", ios::out|ios::trunc ) ;
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
	    int stax = nc_open( "./structT.nc", NC_NOWRITE, &ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to open structT.nc" ;
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
	    CPPUNIT_ASSERT( ndims == 1 ) ;

	    check_var( ncid, "s1", "byte" ) ;
	    check_var( ncid, "s1.s2", "i16" ) ;
	    check_var( ncid, "s1.s2.s3", "i32" ) ;
	    check_var( ncid, "s1", "ui16" ) ;
	    check_var( ncid, "s1.s2", "ui32" ) ;
	    check_var( ncid, "s1.s2.s3", "f32" ) ;
	    check_var( ncid, "s1", "f64" ) ;
	    check_var( ncid, "s1.s2", "str" ) ;

	    stax = nc_close( ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to close structT.nc" ;
		handle_error( stax, err ) ;
	    }

	    CPPUNIT_ASSERT( true ) ;
	}
	catch( BESError &e )
	{
	    cerr << e.get_message() << endl ;
	    CPPUNIT_ASSERT( false ) ;
	}

	try
	{
	    // build a DataDDS of simple types and set values for each of the
	    // simple types.
	    DataDDS *dds = new DataDDS( NULL, "virtual" ) ;

	    Structure s1( "s1" ) ;
	    Structure s2( "s2" ) ;
	    Structure s3( "s3" ) ;

	    Byte b( "byte" ) ;
	    b.set_value( 28 ) ;
	    s1.add_var( &b ) ;

	    Int16 i16( "i16" ) ;
	    i16.set_value( -2048 ) ;
	    s2.add_var( &i16 ) ;

	    Int32 i32( "i32" ) ;
	    i32.set_value( -105467 ) ;
	    s3.add_var( &i32 ) ;

	    UInt16 ui16( "ui16" ) ;
	    ui16.set_value( 2048 ) ;
	    s1.add_var( &ui16 ) ;

	    UInt32 ui32( "ui32" ) ;
	    ui32.set_value( 105467 ) ;
	    s2.add_var( &ui32 ) ;

	    Float32 f32( "f32" ) ;
	    f32.set_value( 5.7866 ) ;
	    s3.add_var( &f32 ) ;

	    Float64 f64( "f64" ) ;
	    f64.set_value( 10245.1234 ) ;
	    s1.add_var( &f64 ) ;

	    Str str( "str" ) ;
	    str.set_value( "This is a String Value" ) ;
	    s2.add_var( &str ) ;

	    s2.add_var( &s3 ) ;
	    s1.add_var( &s2 ) ;

	    dds->add_var( &s1 ) ;

	    // transform the DataDDS into a netcdf file. The dhi only needs the
	    // output stream and the post constraint. Test no constraints and
	    // then some different constraints (1 var, 2 var)

	    // The resulting netcdf file is streamed back. Write this file to a
	    // test file locally
	    BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	    BESDataHandlerInterface dhi ;
	    ofstream fstrm( "./structT.nc", ios::out|ios::trunc ) ;
	    dhi.set_output_stream( &fstrm ) ;
	    dhi.data[POST_CONSTRAINT] = "s1.ui16,s1.s2.str,s1.s2.s3.i32" ;
	    FONcTransmitter ft ;
	    FONcTransmitter::send_data( obj, dhi ) ;
	    fstrm.close() ;

	    // deleting the response object deletes the DataDDS
	    delete obj ;

	    // open the netcdf file and check the contents. NO ATTRIBUTES TO
	    // START WITH.
	    int ncid = 0 ;
	    int stax = nc_open( "./structT.nc", NC_NOWRITE, &ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to open structT.nc" ;
		handle_error( stax, err ) ;
	    }

	    int nvars = 0 ;
	    stax = nc_inq_nvars( ncid, &nvars ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire nvars" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( nvars == 3 ) ;

	    int ndims = 0 ;
	    stax = nc_inq_ndims( ncid, &ndims ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire ndims" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( ndims == 1 ) ;

	    check_var( ncid, "s1.s2.s3", "i32" ) ;
	    check_var( ncid, "s1", "ui16" ) ;
	    check_var( ncid, "s1.s2", "str" ) ;

	    stax = nc_close( ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to close structT.nc" ;
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

CPPUNIT_TEST_SUITE_REGISTRATION( structT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

