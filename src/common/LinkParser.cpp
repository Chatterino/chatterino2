#include "common/LinkParser.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>

// ip 0.0.0.0 - 224.0.0.0
#define IP                                       \
    "(?:[1-9]\\d?|1\\d\\d|2[01]\\d|22[0-3])"     \
    "(?:\\.(?:1?\\d{1,2}|2[0-4]\\d|25[0-5])){2}" \
    "(?:\\.(?:[1-9]\\d?|1\\d\\d|2[0-4]\\d|25[0-4]))"
#define PORT "(?::\\d{2,5})"
#define WEB_CHAR1 "[_a-z\\x{00a1}-\\x{ffff}0-9]"
#define WEB_CHAR2 "[a-z\\x{00a1}-\\x{ffff}0-9]"

#define SPOTIFY_1 "(?:artist|album|track|user:[^:]+:playlist):[a-zA-Z0-9]+"
#define SPOTIFY_2 "user:[^:]+"
#define SPOTIFY_3 "search:(?:[-\\w$\\.+!*'(),]+|%[a-fA-F0-9]{2})+"
#define SPOTIFY_PARAMS "(?:" SPOTIFY_1 "|" SPOTIFY_2 "|" SPOTIFY_3 ")"
#define SPOTIFY_LINK "(?x-mi:(spotify:" SPOTIFY_PARAMS "))"

#define WEB_PROTOCOL "(?:(?:https?|ftps?)://)?"
#define WEB_USER "(?:\\S+(?::\\S*)?@)?"
#define WEB_HOST "(?:(?:" WEB_CHAR1 "-*)*" WEB_CHAR2 "+)"
#define WEB_DOMAIN "(?:\\.(?:" WEB_CHAR2 "-*)*" WEB_CHAR2 "+)*"
#define WEB_TLD "(?:" + tldData + ")"
#define WEB_RESOURCE_PATH "(?:[/?#]\\S*)"
#define WEB_LINK                                                              \
    WEB_PROTOCOL WEB_USER "(?:" IP "|" WEB_HOST WEB_DOMAIN "\\." WEB_TLD PORT \
                          "?" WEB_RESOURCE_PATH "?)"

#define LINK "^(?:" SPOTIFY_LINK "|" WEB_LINK ")$"

namespace chatterino
{
    LinkParser::LinkParser(const QString& unparsedString)
    {
        static QRegularExpression linkRegex = [] {
            static QRegularExpression newLineRegex("\r?\n");
            QFile file(":/tlds.txt");
            file.open(QFile::ReadOnly);
            QTextStream tlds(&file);
            tlds.setCodec("UTF-8");

            // tldData gets injected into the LINK macro
            auto tldData = tlds.readAll().replace(newLineRegex, "|");
            (void)tldData;

            return QRegularExpression(
                LINK, QRegularExpression::CaseInsensitiveOption);
        }();

        this->match_ = linkRegex.match(unparsedString);
    }

    bool LinkParser::hasMatch() const
    {
        return this->match_.hasMatch();
    }

    QString LinkParser::getCaptured() const
    {
        return this->match_.captured();
    }

}  // namespace chatterino
