#include <cppunit/extensions/HelperMacros.h>
#include "protocol.h"
#include <string.h>
#include <stdio.h>
#include "maclist.h"

class ProtocolTest : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE( ProtocolTest );
    CPPUNIT_TEST( testProtocol );
    CPPUNIT_TEST_SUITE_END();
    public:
        void setUp () {};
        void tearDown () {};
        void testProtocol ();
};

void ProtocolTest::testProtocol () 
{
    printf ("Testing Protocol ...\n");
    CPPUNIT_ASSERT_EQUAL((int)checksum((uint16_t *)"aaaaaa", 6), 0xdbdb);
    CPPUNIT_ASSERT_EQUAL((int)checksum((uint16_t *)"AAAAAAAA", 8), 0xfafa);
    CPPUNIT_ASSERT_EQUAL((int)checksum((uint16_t *)"AAAAAAA", 7), 0x3bfb);

    MacList ml("aa:bb:cc:dd:ee:ff");

    struct frame f;
    buildFrame(&f, ml.nextMac(), ml.nextMac(), 12345, 12345, 12345, 12345, "Hello, World!", 14, 14);
    CPPUNIT_ASSERT_EQUAL((int)checksum((uint16_t *)&f, 100), 21661);
}

CPPUNIT_TEST_SUITE_REGISTRATION( ProtocolTest );
