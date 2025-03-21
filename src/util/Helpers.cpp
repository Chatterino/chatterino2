#include "util/Helpers.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchCommon.hpp"

#include <QDateTime>
#include <QDirIterator>
#include <QLocale>
#include <QRegularExpression>
#include <QTimeZone>
#include <QUuid>

namespace {

const QString ZERO_WIDTH_JOINER = QStringLiteral("\u200D");

// Note: \U requires /utf-8 for MSVC
// See https://mm2pl.github.io/emoji_rfc.pdf
const QRegularExpression ESCAPE_TAG_REGEX(
    QStringLiteral("(?<!\U000E0002)\U000E0002"),
    QRegularExpression::UseUnicodePropertiesOption);

}  // namespace

namespace chatterino {

namespace helpers::detail {

    SizeType skipSpace(QStringView view, SizeType startPos)
    {
        while (startPos < view.length() && view.at(startPos).isSpace())
        {
            startPos++;
        }
        return startPos - 1;
    }

    bool matchesIgnorePlural(QStringView word, const QString &expected)
    {
        if (!word.startsWith(expected))
        {
            return false;
        }
        if (word.length() == expected.length())
        {
            return true;
        }
        return word.length() == expected.length() + 1 &&
               word.at(word.length() - 1).toLatin1() == 's';
    }

    std::pair<uint64_t, bool> findUnitMultiplierToSec(QStringView view,
                                                      SizeType &pos)
    {
        // Step 1. find end of unit
        auto startIdx = pos;
        auto endIdx = view.length();
        for (; pos < view.length(); pos++)
        {
            auto c = view.at(pos);
            if (c.isSpace() || c.isDigit())
            {
                endIdx = pos;
                break;
            }
        }
        pos--;

        // TODO(QT6): use sliced (more readable)
        auto unit = view.mid(startIdx, endIdx - startIdx);
        if (unit.isEmpty())
        {
            return std::make_pair(0, false);
        }

        auto first = unit.at(0).toLatin1();
        switch (first)
        {
            case 's': {
                if (unit.length() == 1 ||
                    matchesIgnorePlural(unit, QStringLiteral("second")))
                {
                    return std::make_pair(1, true);
                }
            }
            break;
            case 'm': {
                if (unit.length() == 1 ||
                    matchesIgnorePlural(unit, QStringLiteral("minute")))
                {
                    return std::make_pair(60, true);
                }
                if ((unit.length() == 2 && unit.at(1).toLatin1() == 'o') ||
                    matchesIgnorePlural(unit, QStringLiteral("month")))
                {
                    return std::make_pair(60 * 60 * 24 * 30, true);
                }
            }
            break;
            case 'h': {
                if (unit.length() == 1 ||
                    matchesIgnorePlural(unit, QStringLiteral("hour")))
                {
                    return std::make_pair(60 * 60, true);
                }
            }
            break;
            case 'd': {
                if (unit.length() == 1 ||
                    matchesIgnorePlural(unit, QStringLiteral("day")))
                {
                    return std::make_pair(60 * 60 * 24, true);
                }
            }
            break;
            case 'w': {
                if (unit.length() == 1 ||
                    matchesIgnorePlural(unit, QStringLiteral("week")))
                {
                    return std::make_pair(60 * 60 * 24 * 7, true);
                }
            }
            break;
        }
        return std::make_pair(0, false);
    }

}  // namespace helpers::detail
using namespace helpers::detail;

bool startsWithOrContains(QStringView str1, QStringView str2,
                          Qt::CaseSensitivity caseSensitivity, bool startsWith)
{
    if (startsWith)
    {
        return str1.startsWith(str2, caseSensitivity);
    }

    return str1.contains(str2, caseSensitivity);
}

bool isNeutral(const QString &s)
{
    static const QRegularExpression re("\\p{L}");
    const QRegularExpressionMatch match = re.match(s);
    return !match.hasMatch();
}

QString generateUuid()
{
    auto uuid = QUuid::createUuid();
    return uuid.toString();
}

QString formatRichLink(const QString &url, bool file)
{
    return QString("<a href=\"") + (file ? "file:///" : "") + url + "\">" +
           url + "</a>";
}

QString formatRichNamedLink(const QString &url, const QString &name, bool file)
{
    return QString("<a href=\"") + (file ? "file:///" : "") + url + "\">" +
           name + "</a>";
}

QString shortenString(const QString &str, unsigned maxWidth)
{
    auto shortened = QString(str);

    if (str.size() > int(maxWidth))
    {
        shortened.resize(int(maxWidth));
        shortened += "...";
    }

    return shortened;
}

QString kFormatNumbers(const int &number)
{
    return QString("%1K").arg(number / 1000);
}

QColor getRandomColor(const QString &userId)
{
    bool ok = true;
    int colorSeed = userId.toInt(&ok);
    if (!ok)
    {
        // We were unable to convert the user ID to an integer, this means Twitch started to use non-integer user IDs (or we're on IRC)
        // Use sum of unicode values of all characters in id / IRC nick
        colorSeed = 0;
        for (const auto &c : userId)
        {
            colorSeed += c.digitValue();
        }
    }

    const auto colorIndex = colorSeed % TWITCH_USERNAME_COLORS.size();
    return TWITCH_USERNAME_COLORS[colorIndex];
}

QString formatUserMention(const QString &userName, bool isFirstWord,
                          bool mentionUsersWithComma)
{
    QString result = userName;

    if (isFirstWord && mentionUsersWithComma)
    {
        result += ",";
    }

    return result;
}

int64_t parseDurationToSeconds(const QString &inputString,
                               uint64_t noUnitMultiplier)
{
    if (inputString.length() == 0)
    {
        return -1;
    }

    QStringView input(inputString);
    input = input.trimmed();

    uint64_t currentValue = 0;

    bool visitingNumber = true;  // input must start with a number
    SizeType numberStartIdx = 0;

    for (SizeType pos = 0; pos < input.length(); pos++)
    {
        QChar c = input.at(pos);

        if (visitingNumber && !c.isDigit())
        {
            uint64_t parsed =
                (uint64_t)input.mid(numberStartIdx, pos - numberStartIdx)
                    .toUInt();

            if (c.isSpace())
            {
                pos = skipSpace(input, pos) + 1;
                if (pos >= input.length())
                {
                    // input like "40  ", this shouldn't happen
                    // since we trimmed the view
                    return -1;
                }
                c = input.at(pos);
            }

            auto result = findUnitMultiplierToSec(input, pos);
            if (!result.second)
            {
                return -1;  // invalid unit or leading spaces (shouldn't happen)
            }

            currentValue += parsed * result.first;
            visitingNumber = false;
        }
        else if (!visitingNumber && !c.isSpace())
        {
            if (!c.isDigit())
            {
                return -1;  // expected a digit
            }
            visitingNumber = true;
            numberStartIdx = pos;
        }
        // else: visitingNumber && isDigit || !visitingNumber && isSpace
    }

    if (visitingNumber)
    {
        if (numberStartIdx != 0)
        {
            return -1;  // input like "1w 3s 70", 70 what? apples?
        }
        currentValue += input.toUInt() * noUnitMultiplier;
    }

    return (int64_t)currentValue;
}

bool compareEmoteStrings(const QString &a, const QString &b)
{
    // try comparing insensitively, if they are the same then sensitively
    // (fixes order of LuL and LUL)
    int k = QString::compare(a, b, Qt::CaseInsensitive);
    if (k == 0)
    {
        return a > b;
    }

    return k < 0;
}

QString unescapeZeroWidthJoiner(QString escaped)
{
    escaped.replace(ESCAPE_TAG_REGEX, ZERO_WIDTH_JOINER);
    return escaped;
}

QLocale getSystemLocale()
{
#ifdef CHATTERINO_WITH_TESTS
    if (getApp()->isTest())
    {
        return {QLocale::English};
    }
#endif

    return QLocale::system();
}

QDateTime chronoToQDateTime(std::chrono::system_clock::time_point time)
{
    auto msSinceEpoch =
        std::chrono::time_point_cast<std::chrono::milliseconds>(time)
            .time_since_epoch();
    auto dt = QDateTime::fromMSecsSinceEpoch(msSinceEpoch.count());

#if CHATTERINO_WITH_TESTS
    if (getApp()->isTest())
    {
        dt = dt.toUTC();
    }
#endif

    return dt;
}

QStringView codepointSlice(QStringView str, qsizetype begin, qsizetype end)
{
    if (end <= begin || begin < 0)
    {
        return {};
    }

    qsizetype n = 0;
    const QChar *pos = str.begin();
    const QChar *endPos = str.end();

    const QChar *sliceBegin = nullptr;
    while (n < end)
    {
        if (pos >= endPos)
        {
            return {};
        }
        if (n == begin)
        {
            sliceBegin = pos;
        }

        QChar cur = *pos++;
        if (cur.isHighSurrogate() && pos < endPos && pos->isLowSurrogate())
        {
            pos++;
        }
        n++;
    }
    assert(pos <= endPos);

    return {sliceBegin, pos};
}

void removeFirstQS(QString &str)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    str.removeFirst();
#else
    str.remove(0, 1);
#endif
}

void removeLastQS(QString &str)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    str.removeLast();
#else
    str.chop(1);
#endif
}

}  // namespace chatterino
