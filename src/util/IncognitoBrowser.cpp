#include "util/IncognitoBrowser.hpp"
#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"
#elif defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)
#    include "util/XDGHelper.hpp"
#endif

#include <QProcess>
#include <QVariant>
#include <QFileInfo>

namespace chatterino {

namespace {

constexpr bool isWindows()
{
#if Q_OS_WINDOWS
    return true;
#else
    return false;
#endif
}


QString getExecutable()
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

QString getPrivateArg(const QString &browserPath)
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
        {"chromium", "-incognito"},
        {"brave", "-incognito"},
    }};

    // may return null string if path is '/'
    QString exeFilename = QFileInfo(browserPath).fileName();

    if (!exeFilename.isNull())
    {
        // mozilla distributes many firefox variants with different endings so
        // catch them all here, and be future proof at the same time :)
        if (exeFilename.startsWith("firefox-"))
            return "-private-window";

        for (const auto &pair : argTable)
            if (exeFilename == pair.first)
                return pair.second;
    }

    // unsupported or invalid browser
    return {};
}

}  // namespace

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
