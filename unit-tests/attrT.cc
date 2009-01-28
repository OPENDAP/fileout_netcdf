// attrT.cc

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

char *byteattrs[11] = { 
"byteattr",
"int16attr",
"ui16attr",
"int32attr",
"uint32attr",
"floatattr",
"doubleattr",
"singlebyteattr",
"singlestringattr",
"container.containerattr",
"container.subcontainer.subcontainerattr"
} ;
nc_type bytetypes[11] = { NC_BYTE,NC_SHORT,NC_INT,NC_INT,NC_LONG,NC_FLOAT,NC_DOUBLE,NC_BYTE,NC_CHAR,NC_CHAR,NC_CHAR } ;
size_t bytesize[11] = { 10,11,10,11,11,9,12,1,12,25,29 } ;

char *f32attrs[3] = {
"mystruct.mystructattr1",
"mystruct.mystructattr2",
"mystruct.mystructattr3"
} ;
char *f32attrvals[3] = {
"mystructattr1val",
"mystructattr2val",
"mystructattr3val"
} ;

char *f64attrs[4] = {
"mystruct.mystructattr1",
"mystruct.mystructattr2",
"mystruct.mystructattr3",
"f64attr1"
} ;
char *f64attrvals[4] = {
"mystructattr1val",
"mystructattr2val",
"mystructattr3val",
"f64attr1val"
} ;

char *strattrs[5] = {
"mystruct.mystructattr1",
"mystruct.mystructattr2",
"mystruct.mystructattr3",
"strattr1",
"strattr2"
} ;
char *strattrvals[5] = {
"mystructattr1val",
"mystructattr2val",
"mystructattr3val",
"strattr1val",
"strattr2val"
} ;

char *i16attrs[9] = {
"mystruct.mystructattr1",
"mystruct.mystructattr2",
"mystruct.mystructattr3",
"mystruct.mysubstructure.ssattr1",
"i16attr1",
"i16attr2",
"i16attr3",
"i16attr4",
"i16attr5"
} ;
char *i16attrvals[9] = {
"mystructattr1val",
"mystructattr2val",
"mystructattr3val",
"ssattr1val",
"i16attr1val",
"i16attr2val",
"i16attr3val",
"i16attr4val",
"i16attr5val"
} ;

char *i32attrs[4] = {
"mystruct.mystructattr1",
"mystruct.mystructattr2",
"mystruct.mystructattr3",
"mystruct.mysubstructure.ssattr1"
} ;
char *i32attrvals[4] = {
"mystructattr1val",
"mystructattr2val",
"mystructattr3val",
"ssattr1val"
} ;

class attrT: public TestFixture {
private:
public:
    attrT() {}
    ~attrT() {}

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
	if( !access( "attrT.nc", F_OK ) )
	{
	    remove( "attrT.nc" ) ;
	}
	*/
    }

    CPPUNIT_TEST_SUITE( attrT ) ;

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

    void check_globals( int ncid )
    {
	int natts = 0 ;
	int stax = nc_inq_varnatts( ncid, NC_GLOBAL, &natts ) ;
	if( stax != NC_NOERR )
	{
	    string err = "unable to inquire natts" ;
	    handle_error( stax, err ) ;
	}
	CPPUNIT_ASSERT( natts == 3 ) ;

	for( int ai = 0; ai < natts; ai++ )
	{
	    char name[64] ;
	    stax = nc_inq_attname( ncid, NC_GLOBAL, ai, name ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire attribute name" ;
		handle_error( stax, err ) ;
	    }
	    string sname = name ;
	    nc_type attr_type ;
	    size_t attr_len ;
	    if( sname == "title" )
	    {
		stax = nc_inq_att( ncid, NC_GLOBAL, name,
				   &attr_type, &attr_len ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to inquire about title" ;
		    handle_error( stax, err ) ;
		}
		CPPUNIT_ASSERT( attr_type == NC_CHAR ) ;
		CPPUNIT_ASSERT( attr_len == 18 ) ;
		char val[128] ;
		stax = nc_get_att_text( ncid, NC_GLOBAL, name, val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to get title attribute" ;
		    handle_error( stax, err ) ;
		}
		val[attr_len] = '\0' ;
		string sval = val ;
		CPPUNIT_ASSERT( sval == "Attribute Test DDS" ) ;
	    }
	    else if( sname == "contact" )
	    {
		stax = nc_inq_att( ncid, NC_GLOBAL, name,
				   &attr_type, &attr_len ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to inquire about contact" ;
		    handle_error( stax, err ) ;
		}
		CPPUNIT_ASSERT( attr_type == NC_CHAR ) ;
		CPPUNIT_ASSERT( attr_len == 12 ) ;
		char val[128] ;
		stax = nc_get_att_text( ncid, NC_GLOBAL, name, val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to get title attribute" ;
		    handle_error( stax, err ) ;
		}
		val[attr_len] = '\0' ;
		string sval = val ;
		CPPUNIT_ASSERT( sval == "Patrick West" ) ;
	    }
	    else if( sname == "contact_email" )
	    {
		stax = nc_inq_att( ncid, NC_GLOBAL, name,
				   &attr_type, &attr_len ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to inquire about contact" ;
		    handle_error( stax, err ) ;
		}
		CPPUNIT_ASSERT( attr_type == NC_CHAR ) ;
		CPPUNIT_ASSERT( attr_len == 24 ) ;
		char val[128] ;
		stax = nc_get_att_text( ncid, NC_GLOBAL, name, val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to get title attribute" ;
		    handle_error( stax, err ) ;
		}
		val[attr_len] = '\0' ;
		string sval = val ;
		CPPUNIT_ASSERT( sval == "opendap-tech@opendap.org" ) ;
	    }
	    else
	    {
		CPPUNIT_ASSERT( !"unknown global attribute" ) ;
	    }
	}
    }

    // byteattr = 65b, 66b, 67b, 68b, 69b, 70b, 71b, 72b, 73b, 74b
    // int16attr = -50s, -40s, -30s, -20s, -10s, 0s, 10s, 20s, 30s, 40s, 50s
    // ui16attr = 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000
    // int32attr = -100000, -80000, -60000, -40000, -20000, 0, 20000, 40000, 60000, 80000, 100000
    // uint32attr = 0, 100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000, 1000000
    // floatattr = 0.15f, 5.81f, 11.47f, 17.13f, 22.79f, 28.45f, 34.11f, 39.77f, 45.43f
    // doubleattr = 100.155, 976.423, 1852.69, 2728.96, 3605.23, 4481.49, 5357.76, 6234.03, 7110.3, 7986.57, 8862.83, 9739.1
    // singlebyteattr = 77b
    // singlestringattr = "String Value"
    // container.containerattr = "Container Attribute Value"
    // container.subcontainer.subcontainerattr = "Sub Container Attribute Value"
    void check_byte( int ncid, int varid, int natts )
    {
	int stax = NC_NOERR ;
	nc_type attr_type ;
	size_t attr_len ;
	for( int ai = 0; ai < natts; ai++ )
	{
	    stax = nc_inq_att( ncid, varid, byteattrs[ai], &attr_type, &attr_len ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire about attribute" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( attr_type == bytetypes[ai] ) ;
	    CPPUNIT_ASSERT( attr_len == bytesize[ai] ) ;
	    string sname = byteattrs[ai] ;
	    if( sname == "byteattr" )
	    {
		unsigned char val[attr_len] ;
		stax = nc_get_att_uchar( ncid, varid, byteattrs[ai], val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "Failed to get byte attribute byteattr" ;
		    handle_error( stax, err ) ;
		}
		for( unsigned char vi = 0; vi < 10; vi++ )
		{
		    CPPUNIT_ASSERT( val[vi] == 65+vi ) ;
		}
	    }
	    else if( sname == "int16attr" )
	    {
		short val[attr_len] ;
		stax = nc_get_att_short( ncid, varid, byteattrs[ai], val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "Failed to get byte attribute int16attr" ;
		    handle_error( stax, err ) ;
		}
		int vi = 0 ;
		for( short s = -50; s <= 50; vi++ )
		{
		    CPPUNIT_ASSERT( val[vi] == s ) ;
		    s += 10 ;
		}
	    }
	    else if( sname == "ui16attr" )
	    {
		int val[attr_len] ;
		stax = nc_get_att_int( ncid, varid, byteattrs[ai], val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "Failed to get byte attribute ui16attr" ;
		    handle_error( stax, err ) ;
		}
		int vi = 0 ;
		for( unsigned short us = 1000; us <= 10000; vi++ )
		{
		    CPPUNIT_ASSERT( val[vi] == us ) ;
		    us += 1000 ;
		}
	    }
	    else if( sname == "int32attr" )
	    {
		int val[attr_len] ;
		stax = nc_get_att_int( ncid, varid, byteattrs[ai], val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "Failed to get byte attribute int32attr" ;
		    handle_error( stax, err ) ;
		}
		int vi = 0 ;
		for( int i = -100000; i <= 100000; vi++ )
		{
		    CPPUNIT_ASSERT( val[vi] == i ) ;
		    i += 20000 ;
		}
	    }
	    else if( sname == "uint32attr" )
	    {
		int val[attr_len] ;
		stax = nc_get_att_int( ncid, varid, byteattrs[ai], val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "Failed to get byte attribute uint32attr" ;
		    handle_error( stax, err ) ;
		}
		int vi = 0 ;
		for( unsigned int i = 0; i <= 1000000; vi++ )
		{
		    CPPUNIT_ASSERT( val[vi] == i ) ;
		    i += 100000 ;
		}
	    }
	    else if( sname == "floatattr" )
	    {
		float val[attr_len] ;
		stax = nc_get_att_float( ncid, varid, byteattrs[ai], val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "Failed to get byte attribute floatattr" ;
		    handle_error( stax, err ) ;
		}
		int vi = 0 ;
		for( float f = 0.15; f <= 50.00; vi++ )
		{
		    ostringstream sval ;
		    sval << val[vi] ;
		    ostringstream sf ;
		    sf << f ;
		    CPPUNIT_ASSERT( sval.str() == sf.str() ) ;
		    f += 5.66 ;
		}
	    }
	    else if( sname == "doubleattr" )
	    {
		double val[attr_len] ;
		stax = nc_get_att_double( ncid, varid, byteattrs[ai], val ) ;
		if( stax != NC_NOERR )
		{
		    string err = "Failed to get byte attribute doubleattr" ;
		    handle_error( stax, err ) ;
		}
		int vi = 0 ;
		for( double d = 100.155; d <= 10000; vi++ )
		{
		    ostringstream sval ;
		    sval << val[vi] ;
		    ostringstream sd ;
		    sd << d ;
		    CPPUNIT_ASSERT( sval.str() == sd.str() ) ;
		    d += 876.268 ;
		}
	    }
	    else if( sname == "singlebyteattr" )
	    {
		unsigned char val = 0 ;
		stax = nc_get_att_uchar( ncid, varid, byteattrs[ai], &val );
		if( stax != NC_NOERR )
		{
		    string err = "unable to get singlebyteattr value" ;
		    handle_error( stax, err ) ;
		}
		CPPUNIT_ASSERT( val == 77 ) ;
	    }
	    else if( sname == "singlestringattr" )
	    {
		char getval[128] ;
		stax = nc_get_att_text( ncid, varid, byteattrs[ai], getval ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to get singlestringattr value" ;
		    handle_error( stax, err ) ;
		}
		getval[attr_len] = '\0' ;
		string val = getval ;
		CPPUNIT_ASSERT( val == "String Value" ) ;
	    }
	    else if( sname == "container.containerattr" )
	    {
		char getval[128] ;
		stax = nc_get_att_text( ncid, varid, byteattrs[ai], getval ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to get container.containerattr value" ;
		    handle_error( stax, err ) ;
		}
		getval[attr_len] = '\0' ;
		string val = getval ;
		CPPUNIT_ASSERT( val == "Container Attribute Value" ) ;
	    }
	    else if( sname == "container.subcontainer.subcontainerattr" )
	    {
		char getval[128] ;
		stax = nc_get_att_text( ncid, varid, byteattrs[ai], getval ) ;
		if( stax != NC_NOERR )
		{
		    string err = "unable to get container.subcontainer.subcontainerattr value" ;
		    handle_error( stax, err ) ;
		}
		getval[attr_len] = '\0' ;
		string val = getval ;
		CPPUNIT_ASSERT( val == "Sub Container Attribute Value" ) ;
	    }
	}
    }

    void check_vals( int ncid, int varid, int natts, char **names, char **vals )
    {
	int stax = NC_NOERR ;
	for( int ai = 0; ai < natts; ai++ )
	{
	    char name[64] ;
	    stax = nc_inq_attname( ncid, varid, ai, name ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire attribute name" ;
		handle_error( stax, err ) ;
	    }
	    string sname = name ;
	    nc_type attr_type ;
	    size_t attr_len ;
	    CPPUNIT_ASSERT( sname == names[ai] ) ;

	    string val = vals[ai] ;

	    stax = nc_inq_att( ncid, varid, name, &attr_type, &attr_len ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire about attribute" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( attr_type == NC_CHAR ) ;
	    CPPUNIT_ASSERT( attr_len == val.length() ) ;
	    char getval[128] ;
	    stax = nc_get_att_text( ncid, varid, name, getval ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to get attribute value" ;
		handle_error( stax, err ) ;
	    }
	    getval[attr_len] = '\0' ;
	    CPPUNIT_ASSERT( val == getval ) ;
	}
    }

    void check_var( int ncid, const string &embed, const string &name )
    {
	string fullname ;
	if( !embed.empty() )
	{
	    fullname = embed + "." + name ;
	}
	else
	{
	    fullname = name ;
	}
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

	if( name == "byte" )
	{
	    CPPUNIT_ASSERT( natts == 11 ) ;
	    check_byte( ncid, varid, natts ) ;
	}
	else if( name == "f32" )
	{
	    CPPUNIT_ASSERT( natts == 3 ) ;
	    check_vals( ncid, varid, natts, f32attrs, f32attrvals ) ;
	}
	else if( name == "f64" )
	{
	    CPPUNIT_ASSERT( natts == 4 ) ;
	    check_vals( ncid, varid, natts, f64attrs, f64attrvals ) ;
	}
	else if( name == "str" )
	{
	    CPPUNIT_ASSERT( natts == 5 ) ;
	    check_vals( ncid, varid, natts, strattrs, strattrvals ) ;
	}
	else if( name == "i16" )
	{
	    CPPUNIT_ASSERT( natts == 9 ) ;
	    check_vals( ncid, varid, natts, i16attrs, i16attrvals ) ;
	}
	else if( name == "i32" )
	{
	    CPPUNIT_ASSERT( natts == 4 ) ;
	    check_vals( ncid, varid, natts, i32attrs, i32attrvals ) ;
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
	    AttrTable &attrs = b.get_attr_table() ;
	    for( unsigned char c = 65; c < 75; c++ )
	    {
		ostringstream strm ;
		strm << (int)c ;
		attrs.append_attr( "byteattr", "Byte", strm.str() ) ;
	    }
	    for( short s = -50; s <= 50; )
	    {
		ostringstream strm ;
		strm << (int)s ;
		attrs.append_attr( "int16attr", "Int16", strm.str() ) ;
		s += 10 ;
	    }
	    for( unsigned short us = 1000; us <= 10000; )
	    {
		ostringstream strm ;
		strm << (unsigned int)us ;
		attrs.append_attr( "ui16attr", "UInt16", strm.str() ) ;
		us += 1000 ;
	    }
	    for( int i = -100000; i <= 100000; )
	    {
		ostringstream strm ;
		strm << (int)i ;
		attrs.append_attr( "int32attr", "Int32", strm.str() ) ;
		i += 20000 ;
	    }
	    for( unsigned int i = 0; i <= 1000000; )
	    {
		ostringstream strm ;
		strm << (unsigned int)i ;
		attrs.append_attr( "uint32attr", "UInt32", strm.str() ) ;
		i += 100000 ;
	    }
	    for( float f = 0.15; f <= 50.00; )
	    {
		ostringstream strm ;
		strm << (float)f ;
		attrs.append_attr( "floatattr", "Float32", strm.str() ) ;
		f += 5.66 ;
	    }
	    for( double d = 100.155; d <= 10000; )
	    {
		ostringstream strm ;
		strm << (double)d ;
		attrs.append_attr( "doubleattr", "Float64", strm.str() ) ;
		d += 876.268 ;
	    }
	    {
		ostringstream strm ;
		strm << (int)77 ;
		attrs.append_attr( "singlebyteattr", "Byte", strm.str() ) ;
	    }
	    {
		attrs.append_attr( "singlestringattr", "String", "String Value" ) ;
	    }
	    {
		AttrTable *table = attrs.append_container( "container" ) ;
		table->append_attr( "containerattr", "String",
				    "Container Attribute Value" ) ;
		AttrTable *subtable = table->append_container( "subcontainer" );
		subtable->append_attr( "subcontainerattr", "String",
				       "Sub Container Attribute Value" ) ;
	    }
	    dds->add_var( &b ) ;

	    Structure s( "mystruct" ) ;
	    AttrTable &sattrs = s.get_attr_table() ;
	    sattrs.append_attr( "mystructattr1", "String", "mystructattr1val" );
	    sattrs.append_attr( "mystructattr2", "String", "mystructattr2val" );
	    sattrs.append_attr( "mystructattr3", "String", "mystructattr3val" );

	    Float32 f32( "f32" ) ;
	    f32.set_value( 5.7866 ) ;
	    s.add_var( &f32 ) ;

	    Float64 f64( "f64" ) ;
	    f64.set_value( 10245.1234 ) ;
	    AttrTable &f64attrs = f64.get_attr_table() ;
	    f64attrs.append_attr( "f64attr1", "String", "f64attr1val" ) ;
	    s.add_var( &f64 ) ;

	    Str str( "str" ) ;
	    str.set_value( "This is a String Value" ) ;
	    AttrTable &strattrs = str.get_attr_table() ;
	    strattrs.append_attr( "strattr1", "String", "strattr1val" ) ;
	    strattrs.append_attr( "strattr2", "String", "strattr2val" ) ;
	    s.add_var( &str ) ;

	    Structure ss( "mysubstructure" ) ;
	    AttrTable &ssattrs = ss.get_attr_table() ;
	    ssattrs.append_attr( "ssattr1", "String", "ssattr1val" ) ;

	    Int16 i16( "i16" ) ;
	    i16.set_value( -2048 ) ;
	    AttrTable &i16attrs = i16.get_attr_table() ;
	    i16attrs.append_attr( "i16attr1", "String", "i16attr1val" ) ;
	    i16attrs.append_attr( "i16attr2", "String", "i16attr2val" ) ;
	    i16attrs.append_attr( "i16attr3", "String", "i16attr3val" ) ;
	    i16attrs.append_attr( "i16attr4", "String", "i16attr4val" ) ;
	    i16attrs.append_attr( "i16attr5", "String", "i16attr5val" ) ;
	    ss.add_var( &i16 ) ;

	    Int32 i32( "i32" ) ;
	    i32.set_value( -105467 ) ;
	    ss.add_var( &i32 ) ;

	    s.add_var( &ss ) ;

	    dds->add_var( &s ) ;
	    AttrTable &gattrs = dds->get_attr_table() ;
	    gattrs.append_attr( "title", "String", "Attribute Test DDS" ) ;
	    gattrs.append_attr( "contact", "String", "Patrick West" ) ;
	    gattrs.append_attr( "contact_email", "String", "opendap-tech@opendap.org" ) ;

	    // transform the DataDDS into a netcdf file. The dhi only needs the
	    // output stream and the post constraint. Test no constraints and
	    // then some different constraints (1 var, 2 var)

	    // The resulting netcdf file is streamed back. Write this file to a
	    // test file locally
	    BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	    BESDataHandlerInterface dhi ;
	    ofstream fstrm( "./attrT.nc", ios::out|ios::trunc ) ;
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
	    int stax = nc_open( "./attrT.nc", NC_NOWRITE, &ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to open attrT.nc" ;
		handle_error( stax, err ) ;
	    }

	    int nvars = 0 ;
	    stax = nc_inq_nvars( ncid, &nvars ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire nvars" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( nvars == 6 ) ;

	    int ndims = 0 ;
	    stax = nc_inq_ndims( ncid, &ndims ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to inquire ndims" ;
		handle_error( stax, err ) ;
	    }
	    CPPUNIT_ASSERT( ndims == 1 ) ;

	    check_globals( ncid ) ;

	    check_var( ncid, "", "byte" ) ;
	    check_var( ncid, "mystruct", "f32" ) ;
	    check_var( ncid, "mystruct", "f64" ) ;
	    check_var( ncid, "mystruct", "str" ) ;
	    check_var( ncid, "mystruct.mysubstructure", "i16" ) ;
	    check_var( ncid, "mystruct.mysubstructure", "i32" ) ;

	    stax = nc_close( ncid ) ;
	    if( stax != NC_NOERR )
	    {
		string err = "unable to close attrT.nc" ;
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

CPPUNIT_TEST_SUITE_REGISTRATION( attrT ) ;

int 
main( int, char** )
{
    CppUnit::TextTestRunner runner ;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() ) ;

    bool wasSuccessful = runner.run( "", false )  ;

    return wasSuccessful ? 0 : 1 ;
}

