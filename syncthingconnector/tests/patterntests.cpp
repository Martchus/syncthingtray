#include "../syncthingignorepattern.h"

#include "../../testhelper/helper.h"

#include <cppunit/TestFixture.h>

using namespace std;
using namespace Data;
using namespace CppUtilities;
using namespace CppUtilities::Literals;

using namespace CPPUNIT_NS;

/*!
 * \brief The MiscTests class tests various features of the connector library.
 */
class IgnorePatternTests : public TestFixture {
    CPPUNIT_TEST_SUITE(IgnorePatternTests);
    CPPUNIT_TEST(testBasicMatching);
    CPPUNIT_TEST(testWildcards);
    CPPUNIT_TEST(testCharacterRange);
    CPPUNIT_TEST(testAlternatives);
    CPPUNIT_TEST(testCaseInsensitiveMatching);
    CPPUNIT_TEST(testGreediness);
    CPPUNIT_TEST(testAlternativePathSeparators);
    CPPUNIT_TEST_SUITE_END();

public:
    IgnorePatternTests();

    void testBasicMatching();
    void testWildcards();
    void testCharacterRange();
    void testAlternatives();
    void testCaseInsensitiveMatching();
    void testGreediness();
    void testAlternativePathSeparators();

    void setUp() override;
    void tearDown() override;

private:
};

CPPUNIT_TEST_SUITE_REGISTRATION(IgnorePatternTests);

IgnorePatternTests::IgnorePatternTests()
{
}

//
// test setup
//

void IgnorePatternTests::setUp()
{
}

void IgnorePatternTests::tearDown()
{
}

void IgnorePatternTests::testBasicMatching()
{
    auto p1 = SyncthingIgnorePattern(QStringLiteral("foo"));
    CPPUNIT_ASSERT(!p1.comment);
    CPPUNIT_ASSERT(!p1.caseInsensitive);
    CPPUNIT_ASSERT(p1.ignore);
    CPPUNIT_ASSERT(!p1.allowRemovalOnParentDirRemoval);
    CPPUNIT_ASSERT_EQUAL(QStringLiteral("foo"), p1.glob);
    CPPUNIT_ASSERT(p1.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(!p1.matches(QStringLiteral("foofoo")));
    CPPUNIT_ASSERT(p1.matches(QStringLiteral("foo/foo")));

    auto p14 = SyncthingIgnorePattern(QStringLiteral("/rust/**/target"));
    CPPUNIT_ASSERT(p14.matches(QStringLiteral("rust/formatter/target")));
    CPPUNIT_ASSERT(!p14.matches(QStringLiteral("rust/formatter/target/CACHEDIR.TAG")));

    auto p14a = SyncthingIgnorePattern(QStringLiteral("/rust/**/target/"));
    CPPUNIT_ASSERT(!p14a.matches(QStringLiteral("rust/formatter/target")));
    CPPUNIT_ASSERT(!p14a.matches(QStringLiteral("rust/formatter/target/CACHEDIR.TAG")));

    auto p14b = SyncthingIgnorePattern(QStringLiteral("/c++/**/target"));
    CPPUNIT_ASSERT(p14b.matches(QStringLiteral("c++/cmake/bookmarksync/server/target")));

    auto p14c = SyncthingIgnorePattern(QStringLiteral("/c++/*/target"));
    CPPUNIT_ASSERT(!p14c.matches(QStringLiteral("c++/cmake/bookmarksync/server/target")));

    auto p15 = SyncthingIgnorePattern(QStringLiteral("/fo[\\]o]/bar"));
    CPPUNIT_ASSERT(p15.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(p15.matches(QStringLiteral("fo]/bar")));
    CPPUNIT_ASSERT(!p15.matches(QStringLiteral("fo\\/bar")));

    auto p16 = SyncthingIgnorePattern(QStringLiteral("/fo{o\\},o}/bar"));
    CPPUNIT_ASSERT(p16.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(p16.matches(QStringLiteral("foo}/bar")));
    CPPUNIT_ASSERT(!p16.matches(QStringLiteral("/foo\\,o}/bar")));

    auto p17 = SyncthingIgnorePattern(QStringLiteral("///fo{o\\},o}/bar"));
    CPPUNIT_ASSERT(p17.comment);
    CPPUNIT_ASSERT(!p17.matches(QStringLiteral("foo/bar")));
}

void IgnorePatternTests::testWildcards()
{
    auto p2 = SyncthingIgnorePattern(QStringLiteral("(?d)(?i)foo*"));
    CPPUNIT_ASSERT(p2.allowRemovalOnParentDirRemoval);
    CPPUNIT_ASSERT(p2.caseInsensitive);
    CPPUNIT_ASSERT(p2.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(p2.matches(QStringLiteral("FooBar")));
    CPPUNIT_ASSERT(!p2.matches(QStringLiteral("barfoo")));
    CPPUNIT_ASSERT(p2.matches(QStringLiteral("bar/foo")));
    CPPUNIT_ASSERT(p2.matches(QStringLiteral("bar/foobaz")));
    CPPUNIT_ASSERT(!p2.matches(QStringLiteral("bar/foo/baz")));

    auto p2a = SyncthingIgnorePattern(QStringLiteral("(?d)foo**"));
    CPPUNIT_ASSERT(p2a.allowRemovalOnParentDirRemoval);
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("foobar")));
    CPPUNIT_ASSERT(!p2a.matches(QStringLiteral("barfoo")));
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("bar/foo")));
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("bar/foobaz")));
    CPPUNIT_ASSERT(p2a.matches(QStringLiteral("bar/foo/baz")));

    auto p3 = SyncthingIgnorePattern(QStringLiteral("fo*ar"));
    CPPUNIT_ASSERT(!p3.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(p3.matches(QStringLiteral("foar")));
    CPPUNIT_ASSERT(p3.matches(QStringLiteral("foobar")));
    CPPUNIT_ASSERT(!p3.matches(QStringLiteral("foobaR")));
    CPPUNIT_ASSERT(p3.matches(QStringLiteral("foobaRar")));
    CPPUNIT_ASSERT(p3.matches(QStringLiteral("foo*bar")));

    auto p4 = SyncthingIgnorePattern(QStringLiteral("fo\\*ar"));
    CPPUNIT_ASSERT(!p4.matches(QStringLiteral("foar")));
    CPPUNIT_ASSERT(!p4.matches(QStringLiteral("foobar")));
    CPPUNIT_ASSERT(!p4.matches(QStringLiteral("foo*bar")));
    CPPUNIT_ASSERT(p4.matches(QStringLiteral("fo*ar")));

    auto p5 = SyncthingIgnorePattern(QStringLiteral("te*ne"));
    CPPUNIT_ASSERT(p5.matches(QStringLiteral("telephone")));
    CPPUNIT_ASSERT(p5.matches(QStringLiteral("subdir/telephone")));
    CPPUNIT_ASSERT(!p5.matches(QStringLiteral("tele/phone")));

    auto p6 = SyncthingIgnorePattern(QStringLiteral("te**ne"));
    CPPUNIT_ASSERT(p6.matches(QStringLiteral("telephone")));
    CPPUNIT_ASSERT(p6.matches(QStringLiteral("subdir/telephone")));
    CPPUNIT_ASSERT(p6.matches(QStringLiteral("tele/phone")));

    auto p7 = SyncthingIgnorePattern(QStringLiteral("te??st"));
    CPPUNIT_ASSERT(p7.matches(QStringLiteral("tebest")));
    CPPUNIT_ASSERT(!p7.matches(QStringLiteral("teb/st")));
    CPPUNIT_ASSERT(!p7.matches(QStringLiteral("test")));

    auto p9 = SyncthingIgnorePattern(QStringLiteral("/foo*"));
    CPPUNIT_ASSERT(p9.matches(QStringLiteral("foo")));
    CPPUNIT_ASSERT(p9.matches(QStringLiteral("foobar")));
    CPPUNIT_ASSERT(!p9.matches(QStringLiteral("barfoo")));
    CPPUNIT_ASSERT(!p9.matches(QStringLiteral("bar/foo")));

    auto p10 = SyncthingIgnorePattern(QStringLiteral("/fo?"));
    CPPUNIT_ASSERT(p10.matches(QStringLiteral("foO")));
    CPPUNIT_ASSERT(!p10.matches(QStringLiteral("foO/")));
    CPPUNIT_ASSERT(!p10.matches(QStringLiteral("fo/")));
    CPPUNIT_ASSERT(!p10.matches(QStringLiteral("bar/foO")));

    auto p11 = SyncthingIgnorePattern(QStringLiteral("/c++/**/.gradle"));
    CPPUNIT_ASSERT(p11.matches(QStringLiteral("c++/cmake/android/.gradle")));
    CPPUNIT_ASSERT(!p11.matches(QStringLiteral("c++/cmake/android/build.gradle")));

    auto p12 = SyncthingIgnorePattern(QStringLiteral("/c++/***/.gradle"));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("c++/cmake/android/.gradle")));
    CPPUNIT_ASSERT(!p12.matches(QStringLiteral("c++/cmake/android/build.gradle")));

    auto p13 = SyncthingIgnorePattern(QStringLiteral("!/Saved/***/0817/HL-00-00.sav"));
    CPPUNIT_ASSERT(p13.matches(QStringLiteral("Saved/SaveGames/0817/HL-00-00.sav")));
    CPPUNIT_ASSERT(!p13.matches(QStringLiteral("Saved/SaveGames/0817/HL-00-01.sav")));

    auto p14 = SyncthingIgnorePattern(QStringLiteral("/c++/**/**/.gradle"));
    CPPUNIT_ASSERT(p14.matches(QStringLiteral("c++/cmake/android/.gradle")));
    CPPUNIT_ASSERT(!p14.matches(QStringLiteral("c++/cmake/android/build.gradle")));
    CPPUNIT_ASSERT(p14.matches(QStringLiteral("c++/cmake//.gradle")));
    CPPUNIT_ASSERT(!p14.matches(QStringLiteral("c++/cmake/.gradle")));

    auto p15 = SyncthingIgnorePattern(QStringLiteral("/foo/*/*.tar.*"));
    CPPUNIT_ASSERT(!p15.matches(QStringLiteral("foo/bar/PKGBUILD")));
    CPPUNIT_ASSERT(!p15.matches(QStringLiteral("foo/bar/test.pkgtar.zst")));
    CPPUNIT_ASSERT(p15.matches(QStringLiteral("foo/bar/test.pkg.tar.")));
    CPPUNIT_ASSERT(p15.matches(QStringLiteral("foo/bar/test.pkg.tar.zst")));
}

void IgnorePatternTests::testCharacterRange()
{
    auto p11 = SyncthingIgnorePattern(QStringLiteral("/fo[o0.]/bar"));
    CPPUNIT_ASSERT(p11.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(p11.matches(QStringLiteral("fo0/bar")));
    CPPUNIT_ASSERT(p11.matches(QStringLiteral("fo./bar")));
    CPPUNIT_ASSERT(!p11.matches(QStringLiteral("foO/bar")));
    CPPUNIT_ASSERT(!p11.matches(QStringLiteral("fo?/bar")));

    auto p12 = SyncthingIgnorePattern(QStringLiteral("/f[oA-C0-3.]o/bar"));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("f0o/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("f1o/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("f2o/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("f3o/bar")));
    CPPUNIT_ASSERT(!p12.matches(QStringLiteral("f4o/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(!p12.matches(QStringLiteral("foO/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("fAo/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("fBo/bar")));
    CPPUNIT_ASSERT(p12.matches(QStringLiteral("fCo/bar")));
    CPPUNIT_ASSERT(!p12.matches(QStringLiteral("fDo/bar")));
}

void IgnorePatternTests::testAlternatives()
{
    auto p13 = SyncthingIgnorePattern(QStringLiteral("/f{o,0o0,l}o/{bar,biz}"));
    CPPUNIT_ASSERT(p13.matches(QStringLiteral("foo/bar")));
    CPPUNIT_ASSERT(p13.matches(QStringLiteral("foo/biz")));
    CPPUNIT_ASSERT(!p13.matches(QStringLiteral("f/biz")));
    CPPUNIT_ASSERT(!p13.matches(QStringLiteral("f0o0/bar")));
    CPPUNIT_ASSERT(p13.matches(QStringLiteral("f0o0o/bar")));
    CPPUNIT_ASSERT(!p13.matches(QStringLiteral("f{o,0o0,}o/{bar,biz}")));
}

void IgnorePatternTests::testCaseInsensitiveMatching()
{
    auto p11a = SyncthingIgnorePattern(QStringLiteral("(?i)/fo[o0.]/bar"));
    CPPUNIT_ASSERT(p11a.caseInsensitive);
    CPPUNIT_ASSERT(p11a.matches(QStringLiteral("foO/bar")));
}

void IgnorePatternTests::testGreediness()
{
    auto p8 = SyncthingIgnorePattern(QStringLiteral("fo*ar*y"));
    CPPUNIT_ASSERT(!p8.matches(QStringLiteral("fooy")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("foary")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("foobary")));
    CPPUNIT_ASSERT(!p8.matches(QStringLiteral("foobaRy")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("foobaRary")));
    CPPUNIT_ASSERT(!p8.matches(QStringLiteral("foobaRaRy")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("foobaRaRarxy")));
    CPPUNIT_ASSERT(p8.matches(QStringLiteral("bar/foobaRaRarxy")));
}

void IgnorePatternTests::testAlternativePathSeparators()
{
    auto p18 = SyncthingIgnorePattern(QStringLiteral("!Saved\\Logs"));
    CPPUNIT_ASSERT(!p18.matches(QStringLiteral("Documents\\Saved\\Logs")));
    CPPUNIT_ASSERT(p18.matches(QStringLiteral("Documents\\Saved\\Logs"), QChar('\\')));
    CPPUNIT_ASSERT(!p18.matches(QStringLiteral("Documents/Saved/Logs"), QChar('/'))); // see remarks in doc
    auto p18a = SyncthingIgnorePattern(QStringLiteral("!Saved/Logs"));
    CPPUNIT_ASSERT(p18a.matches(QStringLiteral("Documents\\Saved\\Logs"), QChar('\\')));
    CPPUNIT_ASSERT(p18a.matches(QStringLiteral("Documents/Saved/Logs"), QChar('\\')));
    auto p18b = SyncthingIgnorePattern(QStringLiteral("!/Documents/Saved/Logs"));
    CPPUNIT_ASSERT(p18b.matches(QStringLiteral("Documents\\Saved\\Logs"), QChar('\\')));
    CPPUNIT_ASSERT(p18b.matches(QStringLiteral("Documents/Saved/Logs"), QChar('\\')));
    CPPUNIT_ASSERT(!p18b.matches(QStringLiteral("Saved\\Logs"), QChar('\\')));
    CPPUNIT_ASSERT(!p18b.matches(QStringLiteral("Saved/Logs"), QChar('\\')));
}
