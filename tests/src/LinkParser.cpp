#include "common/LinkParser.hpp"

#include <gtest/gtest.h>
#include <QString>
#include <QStringList>

using namespace chatterino;

TEST(LinkParser, parseDomainLinks)
{
    const QStringList inputs = {
        "https://chatterino.com",
        "http://chatterino.com",
        "chatterino.com",
        "wiki.chatterino.com",
        "https://wiki.chatterino.com",
        "http://chatterino.co.uk",
        "http://a.io",
        "chatterino.com:80",
        "wiki.chatterino.com:80",
        "a.b.c.chatterino.com",
        "https://a.b.c.chatterino.com/foo",
        "http://chatterino.com?foo",
        "http://xd.chatterino.com/#?foo",
        "chatterino.com#foo",
        "1.com",
        "127.0.0.1.com",
        "https://127.0.0.1.com",
    };

    for (const auto &input : inputs)
    {
        LinkParser p(input);
        ASSERT_TRUE(p.hasMatch()) << input.toStdString();
        ASSERT_EQ(p.getCaptured(), input);
    }
}

TEST(LinkParser, parseIpv4Links)
{
    const QStringList inputs = {
        "https://127.0.0.1",
        "http://127.0.0.1",
        "127.0.0.1",
        "127.0.0.1:8080",
        "255.255.255.255",
        "0.0.0.0",
        "1.1.1.1",
        "001.001.01.1",
        "123.246.87.0",
        "196.168.0.1:",
        "196.168.4.2/foo",
        "196.168.4.2?foo",
        "http://196.168.4.0#foo",
        "196.168.4.0/?#foo",
        "196.168.4.0#?/foo",
        "256.255.255.255",
        "http://256.255.255.255",
        "255.256.255.255",
        "255.255.256.255",
        "255.255.255.256",
    };

    for (const auto &input : inputs)
    {
        LinkParser p(input);
        ASSERT_TRUE(p.hasMatch()) << input.toStdString();
        ASSERT_EQ(p.getCaptured(), input);
    }
}

TEST(LinkParser, doesntParseInvalidIpv4Links)
{
    const QStringList inputs = {
        "https://127.0.0.",
        "http://127.0.01",
        "127.0.0000.1",
        "1.",
        ".127.0.0.1",
        "1.2",
        "1",
        "1.2.3",
    };

    for (const auto &input : inputs)
    {
        LinkParser p(input);
        ASSERT_FALSE(p.hasMatch()) << input.toStdString();
    }
}

TEST(LinkParser, doesntParseInvalidLinks)
{
    const QStringList inputs = {
        "h://foo.com",
        "spotify:1234",
        "ftp://chatterino.com",
        "ftps://chatterino.com",
        "spotify://chatterino.com",
        "httpsx://chatterino.com",
        "https:chatterino.com",
        "/chatterino.com",
        "word",
        ".",
        "/",
        "#",
        ":",
        "?",
        "a",
        "://chatterino.com",
        "//chatterino.com",
    };

    for (const auto &input : inputs)
    {
        LinkParser p(input);
        ASSERT_FALSE(p.hasMatch()) << input.toStdString();
    }
}
