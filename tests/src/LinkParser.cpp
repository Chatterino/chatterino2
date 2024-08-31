#include "common/LinkParser.hpp"

#include "common/Literals.hpp"
#include "Test.hpp"

#include <QString>
#include <QStringList>

using namespace chatterino;
using namespace literals;

struct Case {
    // -Wmissing-field-initializers complains otherwise
    // NOLINTBEGIN(readability-redundant-member-init)
    QString protocol{};
    QString host{};
    QString rest{};
    // NOLINTEND(readability-redundant-member-init)

    void check() const
    {
        QStringList prefixes{
            "", "_", "__", "<", "<<", "<_<", "(((", "<*_~(", "**", "~~",
        };
        QStringList suffixes{
            "",   ">",   "?",      "!",  ".",  ",",  ":",  "*",    "~",
            ">>", "?!.", "~~,*!?", "**", ").", "),", ",)", ")),.", ")?",
        };

        for (const auto &prefix : prefixes)
        {
            for (const auto &suffix : suffixes)
            {
                checkSingle(prefix, suffix);
            }
        }
    }

    void checkSingle(const QString &prefix, const QString &suffix) const
    {
        auto link = this->protocol + this->host + this->rest;
        auto input = prefix + link + suffix;
        auto p = linkparser::parse(input);
        ASSERT_TRUE(p.has_value()) << input;

        if (!p)
        {
            return;
        }

        ASSERT_EQ(p->link, link);
        ASSERT_EQ(p->protocol, this->protocol);
        ASSERT_EQ(p->host, this->host);
        ASSERT_EQ(p->rest, this->rest);
        ASSERT_EQ(p->prefix(input), prefix);
        ASSERT_EQ(p->suffix(input), suffix);
        ASSERT_EQ(p->hasPrefix(input), !prefix.isEmpty());
        ASSERT_EQ(p->hasSuffix(input), !suffix.isEmpty());
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
        {"", "wiki.chatterino.com", ":80?foo"},
        {"", "wiki.chatterino.com", ":80#foo"},
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
        {"", "http.cat", "/200()"},
        {"", "a.com", "?()"},
        {"", "a.com", "#()"},
        {"", "a.com", "/__my_user__"},
        {"", "a.b.c.-._.1.com", ""},
        {"", "0123456789.com", ""},
        {"", "ABCDEFGHIJKLMNOPQRSTUVWXYZ.com", ""},
        {"", "abcdefghijklmnopqrstuvwxyz.com", ""},
        {"", "example.com", "/foo(bar)"},
        {"", "example.com", "/foo((bar))"},
        {"", "example.com", "/(f)(o)(o)(b)(a)r"},
        {"", "example.com", "/foobar()()"},
        {"", "example.com", "/foobar()(())baz"},
        {"", "example.com", "/(foo)"},
        {"", "example.com", "/()"},
        // non-ASCII characters are allowed
        {"", u"köln.de"_s, ""},
        {"", u"ü.com"_s, ""},
        {"", u"─.com"_s, ""},
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
        "196.162.a.1",
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
        ":127.0.0.1",
        ">1.2.3.4",
        "?196.162.8.1",
        "!196.162.8.1",
        ".196.162.8.1",
        ",196.162.8.1",
        ":196.162.8.1",
        "+196.162.8.1",
        "196.162.8.1<",
        "196.162.8.1(())",
        "196.162.8.1(",
        "196.162.8.1(!",
        "127.1.1;.com",
        "127.0.-.1",
        "127...",
        "1.1.1.",
        "1.1.1.:80",
    };

    for (const auto &input : inputs)
    {
        auto p = linkparser::parse(input);
        ASSERT_FALSE(p.has_value()) << input;
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
        "pn.:80",
        "pn./foo",
        "pn.#foo",
        "pn.?foo",
        "http/chatterino.com",
        "http/wiki.chatterino.com",
        "http:cat.com",
        "https:cat.com",
        "http:/cat.com",
        "http:/cat.com",
        "https:/cat.com",
        "chatterino.com-",
        "<<>>",
        ">><<",
        "a.com>><<",
        "~~a.com()",
        "https://chatterino.com><https://chatterino.com",
        "<https://chatterino.com><https://chatterino.com>",
        "chatterino.com><chatterino.com",
        "https://chatterino.com><chatterino.com",
        "<chatterino.com><chatterino.com>",
        "<https://chatterino.com><chatterino.com>",
        "info@example.com",
        "user:pass@example.com",
        ":.com",
        "a:.com",
        "1:.com",
        "[a].com",
        "`a`.com",
        "{a}.com",
        "a.com:pass@example.com",
        "@@@.com",
        "%%%.com",
        "*.com",
        "example.com(foo)",
        "example.com()",
    };

    for (const auto &input : inputs)
    {
        auto p = linkparser::parse(input);
        ASSERT_FALSE(p.has_value()) << input;
    }
}
