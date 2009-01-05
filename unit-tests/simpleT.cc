// simpleT.cc

#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/extensions/HelperMacros.h>

using namespace CppUnit ;

#include <fstream>
#include <iostream>

using std::ofstream ;
using std::ios ;
using std::cerr ;
using std::endl ;

#include <BESDebug.h>

class simpleT: public TestFixture {
private:
public:
    simpleT() {}
    ~simpleT() {}

    void setUp()
    {
	BESDebug::SetUp( "cerr,fonc" ) ;
    }

    void tearDown()
    {
    }

    CPPUNIT_TEST_SUITE( simpleT ) ;

    CPPUNIT_TEST( do_test ) ;

    CPPUNIT_TEST_SUITE_END() ;

    void do_test()
    {
	// build a DataDDS of simple types and set values for each of the
	// simple types.

	// transform the DataDDS into a netcdf file. The dhi only needs the
	// output stream and the post constraint. Test no constraints and
	// then some different constraints (1 var, 2 var)

	// The resulting netcdf file is streamed back. Write this file to a
	// test file locally

	// open the netcdf file and check the contents. NO ATTRIBUTES TO
	// START WITH.

	CPPUNIT_ASSERT( true ) ;
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

