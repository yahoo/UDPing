#include <stdio.h>
#include "maclist.h"

#include <cppunit/extensions/HelperMacros.h>
#include "options.h"
#include <string.h>

class MacListTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE( MacListTest );
    CPPUNIT_TEST( testMacList );
    CPPUNIT_TEST_SUITE_END();
    public:
        void setUp () {};
        void tearDown () {};
        void testMacList ();
};

void test_macList (char* macString, int count, char** compareList) {
    printf ("Testing %s\n", macString);
    MacList ml(macString);
    printf ("Number of macs found: %d\n", ml.getNumMacs());
    char buf[100];
    if (count > 0) {
        for (int ix = 0; ix < 10; ix ++) {
            ml.toString(ml.nextMac(), buf);
            CPPUNIT_ASSERT_EQUAL(strcmp(buf, compareList[ix%count]), 0);
        }
    }
    CPPUNIT_ASSERT_EQUAL (ml.getNumMacs(), count);
}

void MacListTest::testMacList () 
{
    printf ("Testing Mac List ...\n");
    char *compareList[3];
    compareList[0] = (char*) "11:22:33:44:55:66";
    compareList[1] = (char*) "aa:bb:cc:dd:ee:ff";
    compareList[2] = (char*) "00:33:55:77:99:aa";
    test_macList((char*) "11:22:33:44:55:66,aa:bb:cc:dd:ee:ff,00:33:55:77:99:aa", 3, compareList);
    test_macList((char*) "11:22:33:44:55:66", 1, compareList);
    test_macList((char*) "44:55:66,aa:bb:cc:dd:ee:ff,00:33:55:77:99:aa", 0, compareList);
}

CPPUNIT_TEST_SUITE_REGISTRATION( MacListTest );
