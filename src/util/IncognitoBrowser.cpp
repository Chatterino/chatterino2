// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/IncognitoBrowser.hpp"
#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"
#elif defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)
#    include "util/XDGHelper.hpp"
#elifdef Q_OS_DARWIN
#    include "util/MacOsHelpers.h"
#endif

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QVariant>

namespace {

using namespace chatterino;

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
#elifdef Q_OS_DARWIN
    return getMacOSDefaultBrowserPath();
#else
    return {};
#endif
}

}  // namespace
//

namespace chatterino::incognitobrowser::detail {

QString getPrivateSwitch(const QString &browserExecutable)
{
    static auto switches = std::vector<std::pair<QString, QString>>{
        {"librewolf", "-private-window"},
        {"waterfox", "-private-window"},
        {"icecat", "-private-window"},
        {"chrome", "-incognito"},
        {"google chrome", "-incognito"},
        {"google chrome beta", "-incognito"},
        {"google chrome canary", "-incognito"},
        {"google-chrome-stable", "-incognito"},
        {"chromium", "-incognito"},
        {"vivaldi", "-incognito"},
        {"opera", "-incognito"},
        {"brave", "-incognito"},
        {"brave browser", "-incognito"},
        {"msedge", "-inprivate"},
        {"microsoft edge", "-inprivate"},
    };

    // the browser executable may be a full path, strip it to its basename and
    // compare case insensitively

    auto lowercasedBrowserExecutable =
        QFileInfo(QDir::cleanPath(browserExecutable)).baseName().toLower();

    for (const auto &switch_ : switches)
    {
        if (lowercasedBrowserExecutable == switch_.first)
        {
            return switch_.second;
        }
    }

    // catch all mozilla distributed variants
    if (lowercasedBrowserExecutable.startsWith("firefox"))
    {
        return "-private-window";
    }

    // couldn't match any browser -> unknown browser
    return {};
}

}  // namespace chatterino::incognitobrowser::detail

namespace chatterino {

using namespace chatterino::incognitobrowser::detail;

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
