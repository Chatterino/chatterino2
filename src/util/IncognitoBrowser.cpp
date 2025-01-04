#include "util/IncognitoBrowser.hpp"
#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"
#elif defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)
#    include "util/XDGHelper.hpp"
#endif

#include <QProcess>
#include <QVariant>

constexpr bool isWindows()
{
#if Q_OS_WINDOWS
    return true;
#else
    return false;
#endif
}

namespace chatterino {

static QString getExecutable()
{
#ifdef USEWINSDK

    using query = getAssociatedExecutable;
    using QueryType = AssociationQueryType;

    QString cmd;

    if ((cmd = query(QueryType::Protocol, L"HTTP")))
        return cmd;
    if ((cmd = query(QueryType::FileExtension, L".html")))
        return cmd;
    if ((cmd = query(QueryType::FileExtension, L".htm")))
        return cmd;

#elif defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)

    auto desktopFile = getDefaultBrowserDesktopFile();

    if (desktopFile.has_value())
    {
        auto entries = desktopFile->getEntries("Desktop Entry");

        auto exec = entries.find("Exec");
        if (exec != entries.end())
            return parseDesktopExecProgram(exec->second.trimmed());
    }

#endif

    // no browser found or platform not supported
    return {};
}

static QString getPrivateArg(const QString &exec)
{
    // list of command line arguments to turn on private browsing
    static std::vector<std::pair<QString, QString>> argTable = {{
        {"firefox", "-private-window"},
        {"librewolf", "-private-window"},
        {"waterfox", "-private-window"},
        {"icecat", "-private-window"},
        {"chrome", "-incognito"},
        {"google-chrome-stable", "-incognito"},
        {"vivaldi", "-incognito"},
        {"opera", "-newprivatetab"},
        {"opera\\launcher", "--private"},
        {"iexplore", "-private"},
        {"msedge", "-inprivate"},
        {"firefox-esr", "-private-window"},
        {"chromium", "-incognito"},
        {"brave", "-incognito"},
        {"firefox-devedition", "-private-window"},
        {"firefox-developer-edition", "-private-window"},
        {"firefox-beta", "-private-window"},
        {"firefox-nightly", "-private-window"},
    }};

    // compare case-insensitively
    QString lcExec = exec.toLower();

    if (isWindows() && lcExec.endsWith(".exe"))
        lcExec.chop(4);

    for (const auto &pair : argTable)
        if (lcExec.endsWith(pair.first))
            return pair.second;

    // unsupported browser
    return {};
}

bool supportsIncognitoLinks()
{
    QString browserExe = getExecutable();
    if (browserExe.isNull())
        return false;

    QString browserArg = getPrivateArg(browserExe);
    if (browserArg.isNull())
        return false;

    return true;
}

bool openLinkIncognito(const QString &link)
{
    QString browserExe = getExecutable();
    QString browserArg = getPrivateArg(browserExe);

    return QProcess::startDetached(browserExe, {browserArg, link});
}

}  // namespace chatterino
