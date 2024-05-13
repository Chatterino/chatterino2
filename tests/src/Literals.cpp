#include "common/Literals.hpp"

#include "Test.hpp"

using namespace chatterino::literals;

// These tests ensure that the behavior of the suffixes is the same across Qt versions.

TEST(Literals, String)
{
    ASSERT_EQ(u""_s, QStringLiteral(""));
    ASSERT_EQ(u"1"_s, QStringLiteral("1"));
    ASSERT_EQ(u"12"_s, QStringLiteral("12"));
    ASSERT_EQ(u"123"_s, QStringLiteral("123"));
}

TEST(Literals, Latin1String)
{
    ASSERT_EQ(""_L1, QLatin1String(""));
    ASSERT_EQ("1"_L1, QLatin1String("1"));
    ASSERT_EQ("12"_L1, QLatin1String("12"));
    ASSERT_EQ("123"_L1, QLatin1String("123"));

    ASSERT_EQ(""_L1, u""_s);
    ASSERT_EQ("1"_L1, u"1"_s);
    ASSERT_EQ("12"_L1, u"12"_s);
    ASSERT_EQ("123"_L1, u"123"_s);

    ASSERT_EQ(QString(""_L1), u""_s);
    ASSERT_EQ(QString("1"_L1), u"1"_s);
    ASSERT_EQ(QString("12"_L1), u"12"_s);
    ASSERT_EQ(QString("123"_L1), u"123"_s);
}

TEST(Literals, ByteArray)
{
    ASSERT_EQ(""_ba, QByteArrayLiteral(""));
    ASSERT_EQ("1"_ba, QByteArrayLiteral("1"));
    ASSERT_EQ("12"_ba, QByteArrayLiteral("12"));
    ASSERT_EQ("123"_ba, QByteArrayLiteral("123"));
}
