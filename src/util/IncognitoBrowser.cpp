#include "util/IncognitoBrowser.hpp"
#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"
#elif defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)
#    include "util/XDGHelper.hpp"
#endif

#include <QProcess>
#include <QVariant>

namespace {

using namespace chatterino;

QString getPrivateSwitch(const QString &browserExecutable)
{
    // list of command line switches to turn on private browsing in browsers
    static auto switches = std::vector<std::pair<QString, QString>>{
        {"firefox", "-private-window"},     {"librewolf", "-private-window"},
        {"waterfox", "-private-window"},    {"icecat", "-private-window"},
        {"chrome", "-incognito"},           {"vivaldi", "-incognito"},
        {"opera", "-newprivatetab"},        {"opera\\launcher", "--private"},
        {"iexplore", "-private"},           {"msedge", "-inprivate"},
        {"firefox-esr", "-private-window"}, {"chromium", "-incognito"},
    };

    // compare case-insensitively
    auto lowercasedBrowserExecutable = browserExecutable.toLower();

#ifdef Q_OS_WINDOWS
    if (lowercasedBrowserExecutable.endsWith(".exe"))
    {
        lowercasedBrowserExecutable.chop(4);
    }
#endif

    for (const auto &switch_ : switches)
    {
        if (lowercasedBrowserExecutable.endsWith(switch_.first))
        {
            return switch_.second;
        }
    }

    // couldn't match any browser -> unknown browser
    return {};
}

QString getDefaultBrowserExecutable()
{
#ifdef USEWINSDK
    // get default browser start command, by protocol if possible, falling back to extension if not
    QString command =
        getAssociatedExecutable(AssociationQueryType::Protocol, L"http");

    if (command.isNull())
    {
        // failed to fetch default browser by protocol, try by file extension instead
        command = getAssociatedExecutable(AssociationQueryType::FileExtension,
                                          L".html");
    }

    if (command.isNull())
    {
        // also try the equivalent .htm extension
        command = getAssociatedExecutable(AssociationQueryType::FileExtension,
                                          L".htm");
    }

    return command;
#elif defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    static QString defaultBrowser = []() -> QString {
        auto desktopFile = getDefaultBrowserDesktopFile();
        if (desktopFile.has_value())
        {
            auto entry = desktopFile->getEntries("Desktop Entry");
            auto exec = entry.find("Exec");
            if (exec != entry.end())
            {
                return parseDesktopExecProgram(exec->second.trimmed());
            }
        }
        return {};
    }();

    return defaultBrowser;
#else
    return {};
#endif
}

}  // namespace

namespace chatterino {

bool supportsIncognitoLinks()
{
    auto browserExe = getDefaultBrowserExecutable();
    return !browserExe.isNull() && !getPrivateSwitch(browserExe).isNull();
}

bool openLinkIncognito(const QString &link)
{
    auto browserExe = getDefaultBrowserExecutable();
    return QProcess::startDetached(browserExe,
                                   {getPrivateSwitch(browserExe), link});
}

}  // namespace chatterino
