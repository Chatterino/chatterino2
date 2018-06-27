#include "common/LinkParser.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>

namespace chatterino {

LinkParser::LinkParser(const QString &unparsedString)
{
    static QRegularExpression linkRegex = [] {
        QFile tldFile(":/tlds.txt");
        tldFile.open(QFile::ReadOnly);

        QTextStream t1(&tldFile);
        t1.setCodec("UTF-8");

        // Read the TLDs in and replace the newlines with pipes
        QString tldData = t1.readAll().replace("\n", "|");

        const QString urlRegExp =
            "^"
            // protocol identifier
            "(?:(?:https?|ftps?)://)?"
            // user:pass authentication
            "(?:\\S+(?::\\S*)?@)?"
            "(?:"
            // IP address dotted notation octets
            // excludes loopback network 0.0.0.0
            // excludes reserved space >= 224.0.0.0
            // excludes network & broacast addresses
            // (first & last IP address of each class)
            "(?:[1-9]\\d?|1\\d\\d|2[01]\\d|22[0-3])"
            "(?:\\.(?:1?\\d{1,2}|2[0-4]\\d|25[0-5])){2}"
            "(?:\\.(?:[1-9]\\d?|1\\d\\d|2[0-4]\\d|25[0-4]))"
            "|"
            // host name
            "(?:(?:[_a-z\\x{00a1}-\\x{ffff}0-9]-*)*[a-z\\x{00a1}-\\x{ffff}0-9]+)"
            // domain name
            "(?:\\.(?:[a-z\\x{00a1}-\\x{ffff}0-9]-*)*[a-z\\x{00a1}-\\x{ffff}0-9]+)*"
            // TLD identifier
            //"(?:\\.(?:[a-z\\x{00a1}-\\x{ffff}]{2,}))"
            "(?:[\\.](?:" +
            tldData +
            "))"
            "\\.?"
            ")"
            // port number
            "(?::\\d{2,5})?"
            // resource path
            "(?:[/?#]\\S*)?"
            "$";

        return QRegularExpression(urlRegExp, QRegularExpression::CaseInsensitiveOption);
    }();

    this->match_ = linkRegex.match(unparsedString);
}

}  // namespace chatterino
