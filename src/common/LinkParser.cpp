#include "common/LinkParser.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QTextStream>

namespace chatterino {

LinkParser::LinkParser(const QString &unparsedString)
{
    /**
     *  (
     *      https?:\\/\\/ <-- Must be http(s)
     *      (?:
     *          [-;:&=\\+\\$,\\w]+      <-- Username (optional)
     *          (?:
     *              @[-;:&=\\+\\$,\\w]+    <-- If a username is present, match an optional "@passsword"
     *          )?
     *      )?
     *      (?:[\\w-]+\\.){1,256}    <-- Infinite domain capture group "a." (eg: a.b.c. is valid)
     *      [\\w-]+        <-- Matches the TLD (.com, .net, etc.). The `.` before the TLD is present from the match above.
     *      (?::\\d+)? <-- Port (optional)
     *      (?:\\/[^\\/]*) <-- Infinite path matching (/a/b/c), no traling slash
     *      *\\/? <-- Optional trailing slash
     *  )
     */

    static QRegularExpression linkRegex(
        "(^(https?:\\/\\/(([-;:&=\\+\\$,\\w]+)(@"
        "([-;:&=\\+\\$,\\w]+))?)?([\\w-]*\\.){1,}"
        "[\\w-]+(:\\d+)?(\\/[^\\/\\n?]*)*\\/?(\\?"
        "(([^=\\n&]+)(?:=([^&\\n]*))?&"
        ")*([^=\\n&]+)(?:=([^&\\n]*))?)?"
        "(\\#[^\\n]+)?)$)",
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
