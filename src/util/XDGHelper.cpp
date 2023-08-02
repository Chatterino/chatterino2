#include "util/XDGHelper.hpp"

#include "util/CombinePath.hpp"
#include "util/XDGDesktopFile.hpp"
#include "util/XDGDirectory.hpp"

#include <QProcess>
#include <qregularexpression.h>
#include <QSettings>
#include <QStringLiteral>
#include <QTextCodec>
#include <QtGlobal>

#include <unordered_set>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace {

using namespace chatterino;

std::optional<XDGDesktopFile> processMimeAppsList(
    QString const &fileName, QStringList &associations,
    std::unordered_set<QString> &denyList)
{
    static const QString mimetype = QStringLiteral("x-scheme-handler/https");
    XDGDesktopFile mimeappsList(fileName);
    // get the list of desktop ids for the given mimetype under the "Default
    // Applications" group in the mimeapps.list file
    auto defaultGroup = mimeappsList[QStringLiteral("Default Applications")];
    auto defaultApps = defaultGroup.find(mimetype);
    if (defaultApps != defaultGroup.cend())
    {
        // for each desktop ID in the list:
        auto desktopIds = defaultApps->second.splitRef(';', Qt::SkipEmptyParts);
        for (auto const &entry : desktopIds)
        {
            auto desktopId = entry.trimmed().toString();

            // if a valid desktop file is found, verify that it is associated
            // with the type. being in the default list gives it an implicit
            // association, so just check that it's not in the denylist
            if (!denyList.contains(desktopId))
            {
                auto desktopFile = XDGDesktopFile::findDesktopId(desktopId);
                // if a valid association is found, we have found the default
                // application
                if (desktopFile.has_value())
                {
                    return desktopFile;
                }
            }
        }
    }

    // no definitive default application found. process added and removed
    // associations, then return empty

    // load any removed associations into the denylist
    auto removedGroup = mimeappsList[QStringLiteral("Removed Associations")];
    auto removedApps = removedGroup.find(mimetype);
    if (removedApps != removedGroup.end())
    {
        auto desktopIds = removedApps->second.splitRef(';', Qt::SkipEmptyParts);
        for (auto const &entry : desktopIds)
        {
            denyList.insert(entry.trimmed().toString());
        }
    }

    // append any created associations to the associations list
    auto addedGroup = mimeappsList[QStringLiteral("Added Associations")];
    auto addedApps = addedGroup.find(mimetype);
    if (addedApps != addedGroup.end())
    {
        auto desktopIds = addedApps->second.splitRef(';', Qt::SkipEmptyParts);
        for (auto const &entry : desktopIds)
        {
            associations.push_back(entry.trimmed().toString());
        }
    }

    return {};
}

std::optional<XDGDesktopFile> searchMimeAppsListsInDirectory(
    QString const &directory, QStringList &associations,
    std::unordered_set<QString> &denyList)
{
    static auto desktopNames = splitEnvironmentVariable("XDG_CURRENT_DESKTOP");
    static QString const desktopFilename = QStringLiteral("%1-mimeapps.list");
    static QString const nonDesktopFilename = QStringLiteral("mimeapps.list");

    // try desktop specific mimeapps.list files first
    for (const auto &desktopName : desktopNames)
    {
        auto fileName =
            combinePath(directory, desktopFilename.arg(desktopName));
        auto defaultApp = processMimeAppsList(fileName, associations, denyList);
        if (defaultApp.has_value())
        {
            return defaultApp;
        }
    }

    // try the generic mimeapps.list
    auto fileName = combinePath(directory, nonDesktopFilename);
    auto defaultApp = processMimeAppsList(fileName, associations, denyList);
    if (defaultApp.has_value())
    {
        return defaultApp;
    }

    // no definitive default application found
    return {};
}

}  // namespace

namespace chatterino {

std::optional<XDGDesktopFile> getDefaultBrowserDesktopFile()
{
    // use xdg-utils if installed
    QProcess xdgSettings;
    xdgSettings.start("xdg-settings", {"get", "default-web-browser"},
                      QIODevice::ReadOnly);
    xdgSettings.waitForFinished();
    if (xdgSettings.error() == QProcess::UnknownError &&
        xdgSettings.exitCode() == 0)
    {
        auto desktopFile = XDGDesktopFile::findDesktopId(
            xdgSettings.readAllStandardOutput().trimmed());
        if (desktopFile.has_value())
        {
            return desktopFile;
        }
    }

    // no xdg-utils, find it manually by searching mimeapps.list files
    QStringList associations;
    std::unordered_set<QString> denyList;

    // config dirs first
    for (const auto &configDir : getXDGDirectories(XDGDirectory::Config))
    {
        auto defaultApp =
            searchMimeAppsListsInDirectory(configDir, associations, denyList);
        if (defaultApp.has_value())
        {
            return defaultApp;
        }
    }

    // data dirs for backwards compatibility
    for (const auto &dataDir : getXDGDirectories(XDGDirectory::Data))
    {
        auto appsDir = combinePath(dataDir, "applications");
        auto defaultApp =
            searchMimeAppsListsInDirectory(appsDir, associations, denyList);
        if (defaultApp.has_value())
        {
            return defaultApp;
        }
    }

    // no mimeapps.list has an explicit default, use the most preferred added
    // association that exists. We could search here for one we support...
    if (!associations.empty())
    {
        for (const auto &desktopId : associations)
        {
            auto desktopFile = XDGDesktopFile::findDesktopId(desktopId);
            if (desktopFile.has_value())
            {
                return desktopFile;
            }
        }
    }

    return {};
}

QString parseExeFromDesktopExecKey(QString execKey)
{
    // string values in desktop files escape all backslashes. This is an
    // independent escaping scheme that must be processed first
    execKey.replace(QStringLiteral("\\\\"), QStringLiteral("\\"));

    if (!execKey.startsWith('"'))
    {
        // not quoted, find the first space
        auto end = execKey.indexOf(' ');
        if (end != -1)
        {
            execKey = execKey.left(end);
        }
    }
    else
    {
        // quoted. find the end quote (if there is none, this just chops off
        // the beginning quote)
        execKey = execKey.mid(1, execKey.indexOf('"', 1));
    }

    // execKey now contains the first token of the command line, which is the
    // executable name. now, there is a second escaping scheme specific to the
    // exec key that must be applied.
    execKey.replace(QRegularExpression(R"(\\(["`$\\]))"), "\\1");

    return execKey;
}

}  // namespace chatterino

#endif
