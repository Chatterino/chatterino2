#include "common/LinkParser.hpp"

#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringView>
#include <QTextStream>

namespace chatterino {
namespace {
    QSet<QString> &tlds()
    {
        static QSet<QString> tlds = [] {
            QFile file(":/tlds.txt");
            file.open(QFile::ReadOnly);
            QTextStream stream(&file);
            // stream.setEncoding(QStringConverter::Encoding::Utf8);
            int safetyMax = 20000;

            QSet<QString> set;

            while (!stream.atEnd())
            {
                auto line = stream.readLine();
                set.insert(line);

                if (safetyMax-- == 0)
                    break;
            }

            return set;
        }();
        return tlds;
    }

    bool isValidHostname(QStringView &host)
    {
        int index = host.lastIndexOf('.');

        return index != -1 &&
               tlds().contains(host.mid(index + 1).toString().toLower());
    }

    bool isValidIpv4(QStringView &host)
    {
        static auto exp = QRegularExpression("^\\d{1,3}(?:\\.\\d{1,3}){3}$");

        return exp.match(host).hasMatch();
    }

#ifdef C_MATCH_IPV6_LINK
    bool isValidIpv6(QStringView &host)
    {
        static auto exp = QRegularExpression("^\\[[a-fA-F0-9:%]+\\]$");

        return exp.match(host).hasMatch();
    }
#endif
}  // namespace

LinkParser::LinkParser(const QString &unparsedString)
{
    this->match_ = unparsedString;

    // This is not implemented with a regex to increase performance.
    // We keep removing parts of the url until there's either nothing left or we fail.
    QStringView l(unparsedString);

    bool hasHttp = false;

    // Protocol `https?://`
    if (l.startsWith(*"https://", Qt::CaseInsensitive))
    {
        hasHttp = true;
        l = l.mid(8);
    }
    else if (l.startsWith(*"http://", Qt::CaseInsensitive))
    {
        hasHttp = true;
        l = l.mid(7);
    }

    // Http basic auth `user:password`.
    // Not supported for security reasons (misleading links)

    // Host `a.b.c.com`
    QStringView host = l;
    bool lastWasDot = true;
    bool inIpv6 = false;

    for (int i = 0; i < l.size(); i++)
    {
        if (l[i] == '.')
        {
            if (lastWasDot == true)  // no double dots ..
                goto error;
            lastWasDot = true;
        }
        else
        {
            lastWasDot = false;
        }

        if (l[i] == ':' && !inIpv6)
        {
            host = l.mid(0, i);
            l = l.mid(i + 1);
            goto parsePort;
        }
        else if (l[i] == '/')
        {
            host = l.mid(0, i);
            l = l.mid(i + 1);
            goto parsePath;
        }
        else if (l[i] == '?')
        {
            host = l.mid(0, i);
            l = l.mid(i + 1);
            goto parseQuery;
        }
        else if (l[i] == '#')
        {
            host = l.mid(0, i);
            l = l.mid(i + 1);
            goto parseAnchor;
        }

        // ipv6
        if (l[i] == '[')
        {
            if (i == 0)
                inIpv6 = true;
            else
                goto error;
        }
        else if (l[i] == ']')
        {
            inIpv6 = false;
        }
    }

    if (lastWasDot)
        goto error;
    else
        goto done;

parsePort:
    // Port `:12345`
    for (int i = 0; i < std::min<int>(5, l.size()); i++)
    {
        if (l[i] == '/')
            goto parsePath;
        else if (l[i] == '?')
            goto parseQuery;
        else if (l[i] == '#')
            goto parseAnchor;

        if (!l[i].isDigit())
            goto error;
    }

    goto done;

parsePath:
parseQuery:
parseAnchor:
    // we accept everything in the path/query/anchor

done:
    // check host
    this->hasMatch_ = isValidHostname(host) || isValidIpv4(host)
#ifdef C_MATCH_IPV6_LINK

                      || (hasHttp && isValidIpv6(host))
#endif
        ;

    if (this->hasMatch_)
    {
        this->match_ = unparsedString;
    }

    return;

error:
    hasMatch_ = false;
}

bool LinkParser::hasMatch() const
{
    return this->hasMatch_;
}

QString LinkParser::getCaptured() const
{
    return this->match_;
}

}  // namespace chatterino
