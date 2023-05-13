#define QT_NO_CAST_FROM_ASCII  // avoids unexpected implicit casts
#include "common/LinkParser.hpp"

#include <QFile>
#include <QSet>
#include <QString>
#include <QStringView>
#include <QTextStream>

namespace {

QSet<QString> &tlds()
{
    static QSet<QString> tlds = [] {
        QFile file(QStringLiteral(":/tlds.txt"));
        file.open(QFile::ReadOnly);
        QTextStream stream(&file);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        // Default encoding of QTextStream is already UTF-8, at least in Qt6
#else
        stream.setCodec("UTF-8");
#endif
        int safetyMax = 20000;

        QSet<QString> set;

        while (!stream.atEnd())
        {
            auto line = stream.readLine();
            set.insert(line);

            if (safetyMax-- == 0)
            {
                break;
            }
        }

        return set;
    }();
    return tlds;
}

bool isValidTld(QStringView tld)
{
    return tlds().contains(tld.toString().toLower());
}

bool isValidIpv4(QStringView host)
{
    // We don't care about the actual value,
    // we only want to verify the ip.

    char16_t sectionValue = 0;  // 0..256
    uint8_t octetNumber = 0;    // 0..4
    uint8_t sectionDigits = 0;  // 0..3
    bool lastWasDot = true;

    for (auto c : host)
    {
        char16_t current = c.unicode();
        if (current == '.')
        {
            if (lastWasDot || octetNumber == 3)
            {
                return false;
            }
            lastWasDot = true;
            octetNumber++;
            sectionValue = 0;
            sectionDigits = 0;
            continue;
        }
        lastWasDot = false;

        if (current > u'9' || current < u'0')
        {
            return false;
        }

        sectionValue = sectionValue * 10 + (current - u'0');
        sectionDigits++;
        if (sectionValue >= 256 || sectionDigits > 3)
        {
            return false;
        }
    }

    return octetNumber == 3 && !lastWasDot;
}

/**
 * @brief Checks if the string starts with a port number.
 * 
 * The value of the port number isn't checked. A port in this implementation
 * can be in the range 0..100'000.
 */
bool startsWithPort(QStringView string)
{
    for (qsizetype i = 0; i < std::min<qsizetype>(5, string.length()); i++)
    {
        char16_t c = string[i].unicode();
        if (i >= 1 && (c == u'/' || c == u'?' || c == u'#'))
        {
            return true;
        }

        if (!string[i].isDigit())
        {
            return false;
        }
    }
    return true;
}

}  // namespace

namespace chatterino {

LinkParser::LinkParser(const QString &unparsedString)
{
    ParsedLink result;
    // This is not implemented with a regex to increase performance.
    QStringView remaining(unparsedString);
    QStringView protocol(remaining);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    QStringView wholeString(unparsedString);
    const auto refFromView = [&](QStringView view) {
        return QStringRef(&unparsedString,
                          static_cast<int>(view.begin() - wholeString.begin()),
                          static_cast<int>(view.size()));
    };
#endif

    // Check protocol for https?://
    if (remaining.startsWith(QStringLiteral("http"), Qt::CaseInsensitive) &&
        remaining.length() >= 4 + 3 + 1)  // 'http' + '://' + [any]
    {
        // optimistic view assuming there's a protocol (http or https)
        auto withProto = remaining.mid(4);  // 'http'

        if (withProto[0] == QChar(u's') || withProto[0] == QChar(u'S'))
        {
            withProto = withProto.mid(1);
        }

        if (withProto.startsWith(QStringLiteral("://")))
        {
            // there's really a protocol => consume it
            remaining = withProto.mid(3);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            result.protocol = {protocol.begin(), remaining.begin()};
#else
            result.protocol =
                refFromView({protocol.begin(), remaining.begin()});
#endif
        }
    }

    // Http basic auth `user:password` isn't supported for security reasons (misleading links)

    // Host `a.b.c.com`
    QStringView host = remaining;
    QStringView rest;
    bool lastWasDot = true;
    int lastDotPos = -1;
    int nDots = 0;

    // Extract the host
    for (int i = 0; i < remaining.size(); i++)
    {
        char16_t currentChar = remaining[i].unicode();
        if (currentChar == u'.')
        {
            if (lastWasDot)  // no double dots ..
            {
                return;
            }
            lastDotPos = i;
            lastWasDot = true;
            nDots++;
        }
        else
        {
            lastWasDot = false;
        }

        // found a port
        if (currentChar == u':')
        {
            host = remaining.mid(0, i);
            rest = remaining.mid(i);
            remaining = remaining.mid(i + 1);

            if (!startsWithPort(remaining))
            {
                return;
            }

            break;
        }

        // we accept everything in the path/query/anchor
        if (currentChar == u'/' || currentChar == u'?' || currentChar == u'#')
        {
            host = remaining.mid(0, i);
            rest = remaining.mid(i);
            break;
        }
    }

    if (lastWasDot || lastDotPos <= 0)
    {
        return;
    }

    // check host/tld
    if ((nDots == 3 && isValidIpv4(host)) ||
        isValidTld(host.mid(lastDotPos + 1)))
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        result.host = host;
        result.rest = rest;
#else
        result.host = refFromView(host);
        result.rest = refFromView(rest);
#endif
        result.source = unparsedString;
        this->result_ = std::move(result);
    }
}

const std::optional<ParsedLink> &LinkParser::result() const
{
    return this->result_;
}

}  // namespace chatterino
