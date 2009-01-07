// simpleT.cc

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

class simpleT: public TestFixture {
private:
public:
    simpleT() {}
    ~simpleT() {}

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
	if( !access( "simpleT.nc", F_OK ) )
	{
	    remove( "simpleT.nc" ) ;
	}
	*/
    }

    CPPUNIT_TEST_SUITE( simpleT ) ;

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
	if( name == "byte" )
	{
	    CPPUNIT_ASSERT( ndims == 0 ) ;
	    unsigned char val = 0 ;
	    stax = nc_get_var1_uchar( ncid, varid, index, &val ) ;
	    if( stax != NC_NOERR )
	    {
		string err = (string)"Failed to get uchar for " + name ;
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
		string err = (string)"Failed to get short for " + name ;
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
		string err = (string)"Failed to get int for " + name ;
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
		string err = (string)"Failed to get short for " + name ;
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
		string err = (string)"Failed to get int for " + name ;
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
		string err = (string)"Failed to get float for " + name ;
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
		string err = (string)"Failed to get double for " + name ;
		handle_error( stax, err ) ;
	    }
	    double baseline = 10245.1234 ;
	    CPPUNIT_ASSERT( val == baseline ) ;
	}
	else if( name == "str" )
	{
	    CPPUNIT_ASSERT( ndims == 1 ) ;
	    char dimname[NC_MAX_NAME + 1] ;
	    string dnbaseline = "str_dim" ;
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

	    Byte b( "byte" ) ;
	    b.set_value( 28 ) ;
	    dds->add_var( &b ) ;

	    Int16 i16( "i16" ) ;
	    i16.set_value( -2048 ) ;
	    dds->add_var( &i16 ) ;

	    Int32 i32( "i32" ) ;
	    i32.set_value( -105467 ) ;
	    dds->add_var( &i32 ) ;

	    UInt16 ui16( "ui16" ) ;
	    ui16.set_value( 2048 ) ;
	    dds->add_var( &ui16 ) ;

	    UInt32 ui32( "ui32" ) ;
	    ui32.set_value( 105467 ) ;
	    dds->add_var( &ui32 ) ;

	    Float32 f32( "f32" ) ;
	    f32.set_value( 5.7866 ) ;
	    dds->add_var( &f32 ) ;

	    Float64 f64( "f64" ) ;
	    f64.set_value( 10245.1234 ) ;
	    dds->add_var( &f64 ) ;

	    Str s( "str" ) ;
	    s.set_value( "This is a String Value" ) ;
	    dds->add_var( &s ) ;

	    // transform the DataDDS into a netcdf file. The dhi only needs the
	    // output stream and the post constraint. Test no constraints and
	    // then some different constraints (1 var, 2 var)

	    // The resulting netcdf file is streamed back. Write this file to a
	    // test file locally
	    BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	    BESDataHandlerInterface dhi ;
	    ofstream fstrm( "./simpleT.nc", ios::out|ios::trunc ) ;
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
	    int stax = nc_open( "./simpleT.nc", NC_NOWRITE, &ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to open simpleT.nc" ;
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

	    check_var( ncid, "byte" ) ;
	    check_var( ncid, "i16" ) ;
	    check_var( ncid, "i32" ) ;
	    check_var( ncid, "ui16" ) ;
	    check_var( ncid, "ui32" ) ;
	    check_var( ncid, "f32" ) ;
	    check_var( ncid, "f64" ) ;
	    check_var( ncid, "str" ) ;

	    stax = nc_close( ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to close simpleT.nc" ;
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
	    // simple types and provide a constraint.
	    DataDDS *dds = new DataDDS( NULL, "virtual" ) ;

	    Byte b( "byte" ) ;
	    b.set_value( 28 ) ;
	    dds->add_var( &b ) ;

	    Int16 i16( "i16" ) ;
	    i16.set_value( -2048 ) ;
	    dds->add_var( &i16 ) ;

	    Int32 i32( "i32" ) ;
	    i32.set_value( -105467 ) ;
	    dds->add_var( &i32 ) ;

	    UInt16 ui16( "ui16" ) ;
	    ui16.set_value( 2048 ) ;
	    dds->add_var( &ui16 ) ;

	    UInt32 ui32( "ui32" ) ;
	    ui32.set_value( 105467 ) ;
	    dds->add_var( &ui32 ) ;

	    Float32 f32( "f32" ) ;
	    f32.set_value( 5.7866 ) ;
	    dds->add_var( &f32 ) ;

	    Float64 f64( "f64" ) ;
	    f64.set_value( 10245.1234 ) ;
	    dds->add_var( &f64 ) ;

	    Str s( "str" ) ;
	    s.set_value( "This is a String Value" ) ;
	    dds->add_var( &s ) ;

	    // transform the DataDDS into a netcdf file. The dhi only needs the
	    // output stream and the post constraint. Test no constraints and
	    // then some different constraints (1 var, 2 var)

	    // The resulting netcdf file is streamed back. Write this file to a
	    // test file locally
	    BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	    BESDataHandlerInterface dhi ;
	    ofstream fstrm( "./simpleT.nc", ios::out|ios::trunc ) ;
	    dhi.set_output_stream( &fstrm ) ;
	    dhi.data[POST_CONSTRAINT] = "i16,f64,str" ;
	    FONcTransmitter ft ;
	    FONcTransmitter::send_data( obj, dhi ) ;
	    fstrm.close() ;

	    // deleting the response object deletes the DataDDS
	    delete obj ;

	    // open the netcdf file and check the contents. NO ATTRIBUTES TO
	    // START WITH.
	    int ncid = 0 ;
	    int stax = nc_open( "./simpleT.nc", NC_NOWRITE, &ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to open simpleT.nc" ;
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

	    check_var( ncid, "i16" ) ;
	    check_var( ncid, "f64" ) ;
	    check_var( ncid, "str" ) ;

	    stax = nc_close( ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to close simpleT.nc" ;
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
	    // simple types and provide a constraint.
	    DataDDS *dds = new DataDDS( NULL, "virtual" ) ;

	    Byte b( "byte" ) ;
	    b.set_value( 28 ) ;
	    dds->add_var( &b ) ;

	    Int16 i16( "i16" ) ;
	    i16.set_value( -2048 ) ;
	    dds->add_var( &i16 ) ;

	    Int32 i32( "i32" ) ;
	    i32.set_value( -105467 ) ;
	    dds->add_var( &i32 ) ;

	    UInt16 ui16( "ui16" ) ;
	    ui16.set_value( 2048 ) ;
	    dds->add_var( &ui16 ) ;

	    UInt32 ui32( "ui32" ) ;
	    ui32.set_value( 105467 ) ;
	    dds->add_var( &ui32 ) ;

	    Float32 f32( "f32" ) ;
	    f32.set_value( 5.7866 ) ;
	    dds->add_var( &f32 ) ;

	    Float64 f64( "f64" ) ;
	    f64.set_value( 10245.1234 ) ;
	    dds->add_var( &f64 ) ;

	    Str s( "str" ) ;
	    s.set_value( "This is a String Value" ) ;
	    dds->add_var( &s ) ;

	    // transform the DataDDS into a netcdf file. The dhi only needs the
	    // output stream and the post constraint. Test no constraints and
	    // then some different constraints (1 var, 2 var)

	    // The resulting netcdf file is streamed back. Write this file to a
	    // test file locally
	    BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	    BESDataHandlerInterface dhi ;
	    ofstream fstrm( "./simpleT.nc", ios::out|ios::trunc ) ;
	    dhi.set_output_stream( &fstrm ) ;
	    dhi.data[POST_CONSTRAINT] = "byte" ;
	    FONcTransmitter ft ;
	    FONcTransmitter::send_data( obj, dhi ) ;
	    fstrm.close() ;

	    // deleting the response object deletes the DataDDS
	    delete obj ;

	    // open the netcdf file and check the contents. NO ATTRIBUTES TO
	    // START WITH.
	    int ncid = 0 ;
	    int stax = nc_open( "./simpleT.nc", NC_NOWRITE, &ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to open simpleT.nc" ;
		handle_error( stax, err ) ;
	    }

	    int nvars = 0 ;
	    stax = nc_inq_nvars( ncid, &nvars ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire nvars" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( nvars == 1 ) ;

	    int ndims = 0 ;
	    stax = nc_inq_ndims( ncid, &ndims ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire ndims" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( ndims == 0 ) ;

	    check_var( ncid, "byte" ) ;

	    stax = nc_close( ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to close simpleT.nc" ;
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

CPPUNIT_TEST_SUITE_REGISTRATION( simpleT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

