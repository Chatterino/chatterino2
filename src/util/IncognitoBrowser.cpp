#include "util/IncognitoBrowser.hpp"
#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"
#elif defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)
#    include "util/XDGHelper.hpp"
#endif

#include <QFileInfo>
#include <QProcess>
#include <QVariant>

namespace {

using namespace chatterino;

QString getPrivateArg(const QString &exePath)
{
    struct Entry {
        QString exe;
        QString arg;
    };

    static std::vector<Entry> lut{
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
    };

    QString exe = QFileInfo(exePath).baseName().toLower();

#ifdef Q_OS_WINDOWS
    if (exe.endsWith(".exe"))
    {
        exe.chop(4);
    }
#endif

    for (const auto &entry : lut)
    {
        if (exe == entry.exe)
        {
            return entry.arg;
        }
    }

    // catch all mozilla distributed variants
    if (exe.startsWith("firefox"))
    {
        return "-private-window";
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
    return !browserExe.isNull() && !getPrivateArg(browserExe).isNull();
}

bool openLinkIncognito(const QString &link)
{
    auto browserExe = getDefaultBrowserExecutable();
    return QProcess::startDetached(browserExe,
                                   {getPrivateArg(browserExe), link});
}

}  // namespace chatterino
