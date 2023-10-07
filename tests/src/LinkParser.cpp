#include "common/LinkParser.hpp"

#include <gtest/gtest.h>
#include <QString>
#include <QStringList>

using namespace chatterino;

struct Case {
    QString protocol{};
    QString host{};
    QString rest{};

    void check() const
    {
        auto input = this->protocol + this->host + this->rest;
        LinkParser p(input);
        ASSERT_TRUE(p.result().has_value()) << input.toStdString();

        const auto &r = *p.result();
        ASSERT_EQ(r.source, input);
        ASSERT_EQ(r.protocol, this->protocol) << this->protocol.toStdString();
        ASSERT_EQ(r.host, this->host) << this->host.toStdString();
        ASSERT_EQ(r.rest, this->rest) << this->rest.toStdString();
    }
};

TEST(LinkParser, parseDomainLinks)
{
    const QList<Case> cases = {
        {"https://", "chatterino.com"},
        {"http://", "chatterino.com"},
        {"", "chatterino.com"},
        {"", "wiki.chatterino.com"},
        {"https://", "wiki.chatterino.com"},
        {"http://", "chatterino.co.uk"},
        {"http://", "a.io"},
        {"", "chatterino.com", ":80"},
        {"", "wiki.chatterino.com", ":80"},
        {"", "wiki.chatterino.com", ":80/foo/bar"},
        {"", "wiki.chatterino.com", "/:80?foo/bar"},
        {"", "wiki.chatterino.com", "/127.0.0.1"},
        {"", "a.b.c.chatterino.com"},
        {"https://", "a.b.c.chatterino.com", "/foo"},
        {"http://", "chatterino.com", "?foo"},
        {"http://", "xd.chatterino.com", "/#?foo"},
        {"", "chatterino.com", "#foo"},
        {"", "1.com"},
        {"", "127.0.0.1.com"},
        {"https://", "127.0.0.1.com"},
        {"https://", "http.cat", "/200"},
        {"https://", "http.cat"},
        {"", "http.cat", ":8080"},
        {"", "http.cat"},
        {"", "https.cat"},
        {"", "httpsd.cat"},
        {"", "http.cat", "/200"},
        // test case-insensitiveness
        {"HtTpS://", "127.0.0.1.CoM"},
        {"HTTP://", "XD.CHATTERINO.COM", "/#?FOO"},
        {"HTTPS://", "wikI.chatterino.com"},
        {"", "chatterino.Org", "#foo"},
        {"", "CHATTERINO.com", ""},
    };

    for (const auto &c : cases)
    {
        c.check();
    }
}

TEST(LinkParser, parseIpv4Links)
{
    const QList<Case> cases = {
        {"https://", "127.0.0.1"},
        {"http://", "127.0.0.1"},
        {"", "127.0.0.1"},
        {"", "127.0.0.1", ":8080"},
        {"", "255.255.255.255"},
        {"", "0.0.0.0"},
        {"", "1.1.1.1"},
        {"", "001.001.01.1"},
        {"", "123.246.87.0"},
        {"", "196.168.0.1", ":"},
        {"", "196.168.4.2", "/foo"},
        {"", "196.168.4.2", "?foo"},
        {"http://", "196.168.4.0", "#foo"},
        {"", "196.168.4.0", "/?#foo"},
        {"", "196.168.4.0", "#?/foo"},
        // test case-insensitiveness
        {"HTTP://", "196.168.4.0", "#Foo"},
        {"HTTPS://", "196.168.4.0", "#Foo"},
        {"htTp://", "127.0.0.1"},
        {"httpS://", "127.0.0.1"},

    };

    for (const auto &c : cases)
    {
        c.check();
    }
}

TEST(LinkParser, doesntParseInvalidIpv4Links)
{
    const QStringList inputs = {
        // U+0660 - in category "number digits"
        QStringLiteral("٠.٠.٠.٠"),
        "https://127.0.0.",
        "http://127.0.01",
        "127.0.0000.1",
        "1.",
        ".127.0.0.1",
        "1.2",
        "1",
        "1.2.3",
        "htt://256.255.255.255",
        "aliens://256.255.255.255",
        "256.255.255.255",
        "http://256.255.255.255",
        "255.256.255.255",
        "255.255.256.255",
        "255.255.255.256",
    };

    for (const auto &input : inputs)
    {
        LinkParser p(input);
        ASSERT_FALSE(p.result().has_value()) << input.toStdString();
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
        "https:/chatterino.com",
        "http:/chatterino.com",
        "htp://chatterino.com",
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
        "http://pn.",
        "http://pn./",
        "https://pn./",
        "pn./",
        "pn.",
        "http/chatterino.com",
        "http/wiki.chatterino.com",
        "http:cat.com",
        "https:cat.com",
        "http:/cat.com",
        "http:/cat.com",
        "https:/cat.com",
    };

    for (const auto &input : inputs)
    {
        LinkParser p(input);
        ASSERT_FALSE(p.result().has_value()) << input.toStdString();
    }
}
