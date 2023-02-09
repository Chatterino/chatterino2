#include "util/IncognitoBrowser.hpp"

#include <QProcess>
#include <QRegularExpression>
#include <QVariant>

#if defined(Q_OS_WIN) and defined(USEWINSDK)
#    include <Shlwapi.h>
#    include <VersionHelpers.h>

typedef HRESULT(CALLBACK *AssocQueryString_)(ASSOCF, ASSOCSTR, LPCWSTR, LPCWSTR,
                                             LPWSTR, DWORD *);
#endif

namespace {

using namespace chatterino;

#if defined(Q_OS_WIN) and defined(USEWINSDK)
QString injectPrivateSwitch(QString command)
{
    // list of command line switches to turn on private browsing in browsers
    static auto switches = std::vector<std::pair<QString, QString>>{
        {"firefox", "-private-window"},  {"librewolf", "-private-window"},
        {"waterfox", "-private-window"}, {"icecat", "-private-window"},
        {"chrome", "-incognito"},        {"vivaldi", "-incognito"},
        {"opera", "-newprivatetab"},     {"opera\\\\launcher", "--private"},
        {"iexplore", "-private"},        {"msedge", "-inprivate"},
    };

    // transform into regex and replacement string
    std::vector<std::pair<QRegularExpression, QString>> replacers;
    for (const auto &switch_ : switches)
    {
        replacers.emplace_back(
            QRegularExpression("(" + switch_.first + "\\.exe\"?).*",
                               QRegularExpression::CaseInsensitiveOption),
            "\\1 " + switch_.second);
    }

    // try to find matching regex and apply it
    for (const auto &replacement : replacers)
    {
        if (replacement.first.match(command).hasMatch())
        {
            command.replace(replacement.first, replacement.second);
            return command;
        }
    }

    // couldn't match any browser -> unknown browser
    return QString();
}

QString callAssocQueryString(ASSOCF flags, ASSOCSTR str, LPCWSTR pszAssoc,
                             LPCWSTR pszExtra)
{
    static HINSTANCE shlwapi = LoadLibrary(L"shlwapi");
    static auto assocQueryString = AssocQueryString_(
        (shlwapi == NULL) ? NULL
                          : GetProcAddress(shlwapi, "AssocQueryStringW"));

    if (assocQueryString == NULL)
    {
        return QString();
    }

    // always error out instead of returning a truncated string when the
    // buffer is too small - avoids race condition when the user changes their
    // default browser between calls to AssocQueryString
    flags |= ASSOCF_NOTRUNCATE;

    DWORD resultSize = 0;
    assocQueryString(flags, str, pszAssoc, pszExtra, NULL, &resultSize);
    if (resultSize == 0)
    {
        return QString();
    }

    QString result;
    auto buf = new TCHAR[resultSize];
    if (SUCCEEDED(
            assocQueryString(flags, str, pszAssoc, pszExtra, buf, &resultSize)))
    {
        result = QString::fromWCharArray(buf, resultSize);
    }
    delete[] buf;
    return result;
}

QString getCommand()
{
    // get default browser start command, by protocol if possible, falling back to extension if not
    QString command;

    // ASSOCF_IS_PROTOCOL was introduced in Windows 8
    if (IsWindows8OrGreater())
    {
        command = callAssocQueryString(ASSOCF_IS_PROTOCOL | ASSOCF_VERIFY,
                                       ASSOCSTR_COMMAND, L"http", NULL);
    }

    if (command.isNull())
    {
        // failed to fetch default browser by protocol, try by file extension instead
        command = callAssocQueryString(ASSOCF_VERIFY, ASSOCSTR_COMMAND,
                                       L".html", NULL);
    }

    if (command.isNull())
    {
        // also try the equivalent .htm extension
        command = callAssocQueryString(ASSOCF_VERIFY, ASSOCSTR_COMMAND, L".htm",
                                       NULL);
    }

    if (command.isNull())
    {
        // failed to find browser command
        return QString();
    }

    // inject switch to enable private browsing
    command = injectPrivateSwitch(command);
    if (command.isNull())
    {
        return QString();
    }

    return command;
}
#endif

}  // namespace

namespace chatterino {

bool supportsIncognitoLinks()
{
#if defined(Q_OS_WIN) and defined(USEWINSDK)
    return !getCommand().isNull();
#else
    return false;
#endif
}

bool openLinkIncognito(const QString &link)
{
#if defined(Q_OS_WIN) and defined(USEWINSDK)
    auto command = getCommand();

    // TODO: split command into program path and incognito argument
    return QProcess::startDetached(command, {link});
#else
    return false;
#endif
}

}  // namespace chatterino
