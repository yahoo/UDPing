#include <cppunit/extensions/HelperMacros.h>
#include "options.h"
#include <string.h>
#include <stdio.h>

class OptionsTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE( OptionsTest );
    CPPUNIT_TEST( testOptions );
    CPPUNIT_TEST_SUITE_END();
    public:
        void setUp () {};
        void tearDown () {};
        void testOptions ();
};

void OptionsTest::testOptions () 
{
    printf ("Testing Options ...\n");
    char *optarg[6];
    optarg[0] = (char*) "foo";
    optarg[1] = (char*) "-a";
    optarg[2] = (char*) "1";
    optarg[3] = (char*) "-b";
    optarg[4] = (char*) "x";
    optarg[5] = (char*) "-x";
    Options options(6, optarg, "a:b:x");
    CPPUNIT_ASSERT_EQUAL(options.parseIntOption('a', 1, 0, 1, "", ""), 1);
    CPPUNIT_ASSERT_EQUAL(strcmp(options.getStringOption('b', 1, "", ""), "x"), 0);
    CPPUNIT_ASSERT_EQUAL(options.getFlagOption('x'), 1);
    CPPUNIT_ASSERT_EQUAL(options.getFlagOption('y'), 0);
    CPPUNIT_ASSERT_EQUAL(options.getQuiet(), 0);
    CPPUNIT_ASSERT_EQUAL(options.getVerbose(), 0);
}

CPPUNIT_TEST_SUITE_REGISTRATION( OptionsTest );
