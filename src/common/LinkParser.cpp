#include "common/LinkParser.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>

namespace chatterino {

LinkParser::LinkParser(const QString &unparsedString)
{
    static QRegularExpression linkRegex(
        "^(?:http(s)?:\\/\\/)?[\\w.-]+(?:\\.[\\w\\.-]+)+[\\w\\-\\._~:/"
        "?#[\\]@!\\$&'\\(\\)\\*\\+,;=.]+$",
        QRegularExpression::CaseInsensitiveOption);

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
