#include <iostream>
#include <fstream>

using std::cout ;
using std::endl ;
using std::ofstream ;

#include <Connect.h>
#include <DataDDS.h>
#include <BaseTypeFactory.h>
#include <Response.h>
#include <Sequence.h>
#include <Error.h>
#include <DataDDS.h>
#include <DDS.h>

using namespace libdap ;

#include <BESDataDDSResponse.h>
#include <BESDataHandlerInterface.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include "test_config.h"
#include "FONcTransmitter.h"

static void
set_read( DDS *dds )
{
    for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++)
    {
        BaseType *v = *i;
	v->set_read_p( true ) ;
    }
}

static void
print_data( DDS *dds, bool print_rows )
{
    cout << "The data:" << endl;

    for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
        BaseType *v = *i;
        if (print_rows && (*i)->type() == dods_sequence_c)
            dynamic_cast < Sequence * >(*i)->print_val_by_rows(cout);
        else
            v->print_val(cout);
    }

    cout << endl << flush;
}

int
main( int argc, char **argv )
{
    bool debug = false ;
    string file ;
    if( argc > 1 )
    {
	for( int i = 0; i < argc; i++ )
	{
	    string arg = argv[i] ;
	    if( arg == "debug" )
	    {
		debug = true ;
	    }
	    else
	    {
		file = arg ;
	    }
	}
    }
    if( file.empty() )
    {
	cout << "Must specify a file to load" << endl ;
	return 1 ;
    }

    string bes_conf = (string)"BES_CONF=" + TEST_SRC_DIR + "/bes.conf" ;
    putenv( (char *)bes_conf.c_str() ) ;
    if( debug ) BESDebug::SetUp( "cerr,fonc" ) ;

    string fpath = (string)TEST_SRC_DIR + "/data/" + file ;
    string opath = file + ".nc" ;

    Connect *url = 0 ;
    Response *r = 0 ;
    BaseTypeFactory factory ;
    DataDDS *dds = new DataDDS( &factory ) ;
    try
    {
	url = new Connect( fpath ) ;
	r = new Response(fopen( fpath.c_str(), "r"), 0);

	if (!r->get_stream())
	{
	    cout << "Failed to create stream for " << fpath << endl ;
	    return 1 ;
	}

	url->read_data_no_mime( *dds, r ) ;

	if( debug ) print_data( dds, true ) ;
    }
    catch (Error & e)
    {
	cout << e.get_error_message() << endl;
	if( r ) delete r ;
	if( url ) delete url ;
	return 1 ;
    }
    if( r ) delete r ;
    if( url ) delete url ;

    set_read( dds ) ;

    try
    {
	// transform the DataDDS into a netcdf file. The dhi only needs the
	// output stream and the post constraint. Test no constraints and
	// then some different constraints (1 var, 2 var)

	// The resulting netcdf file is streamed back. Write this file to a
	// test file locally
	BESResponseObject *obj = new BESDataDDSResponse( dds ) ;
	BESDataHandlerInterface dhi ;
	ofstream fstrm( opath.c_str(), ios::out|ios::trunc ) ;
	dhi.set_output_stream( &fstrm ) ;
	dhi.data[POST_CONSTRAINT] = "" ;
	FONcTransmitter ft ;
	FONcTransmitter::send_data( obj, dhi ) ;
	fstrm.close() ;

	// deleting the response object deletes the DataDDS
	// FIXME: This causes an illegal free, why?
	// delete obj ;
    }
    catch( BESError &e )
    {
	cout << e.get_message() << endl ;
	return 1 ;
    }

    return 0 ;
}

