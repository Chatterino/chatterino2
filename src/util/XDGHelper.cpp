#include "util/XDGHelper.hpp"

#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "util/CombinePath.hpp"
#include "util/XDGDesktopFile.hpp"
#include "util/XDGDirectory.hpp"

#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStringLiteral>
#include <QTextCodec>
#include <QtGlobal>

#include <unordered_set>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

using namespace chatterino::literals;

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoXDG;

using namespace chatterino;

const auto HTTPS_MIMETYPE = u"x-scheme-handler/https"_s;

/// Read the given mimeapps file and try to find an association for the HTTPS_MIMETYPE
///
/// If the mimeapps file is invalid (i.e. wasn't read), return nullopt
/// If the file is valid, look for the default Desktop File ID handler for the HTTPS_MIMETYPE
/// If no default Desktop File ID handler is found, populate `associations`
///   and `denyList` with Desktop File IDs from "Added Associations" and "Removed Associations" respectively
std::optional<XDGDesktopFile> processMimeAppsList(
    const QString &mimeappsPath, QStringList &associations,
    std::unordered_set<QString> &denyList)
{
    XDGDesktopFile mimeappsFile(mimeappsPath);
    if (!mimeappsFile.isValid())
    {
        return {};
    }

    // get the list of Desktop File IDs for the given mimetype under the "Default
    // Applications" group in the mimeapps.list file
    auto defaultGroup = mimeappsFile.getEntries("Default Applications");
    auto defaultApps = defaultGroup.find(HTTPS_MIMETYPE);
    if (defaultApps != defaultGroup.cend())
    {
        // for each desktop ID in the list:
        auto desktopIds = defaultApps->second.split(';', Qt::SkipEmptyParts);
        for (const auto &entry : desktopIds)
        {
            auto desktopId = entry.trimmed();

            // if a valid desktop file is found, verify that it is associated
            // with the type. being in the default list gives it an implicit
            // association, so just check that it's not in the denylist
            if (!denyList.contains(desktopId))
            {
                auto desktopFile = XDGDesktopFile::findDesktopFile(desktopId);
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
    auto removedGroup = mimeappsFile.getEntries("Removed Associations");
    auto removedApps = removedGroup.find(HTTPS_MIMETYPE);
    if (removedApps != removedGroup.end())
    {
        auto desktopIds = removedApps->second.split(';', Qt::SkipEmptyParts);
        for (const auto &entry : desktopIds)
        {
            denyList.insert(entry.trimmed());
        }
    }

    // append any created associations to the associations list
    auto addedGroup = mimeappsFile.getEntries("Added Associations");
    auto addedApps = addedGroup.find(HTTPS_MIMETYPE);
    if (addedApps != addedGroup.end())
    {
        auto desktopIds = addedApps->second.split(';', Qt::SkipEmptyParts);
        for (const auto &entry : desktopIds)
        {
            associations.push_back(entry.trimmed());
        }
    }

    return {};
}

std::optional<XDGDesktopFile> searchMimeAppsListsInDirectory(
    const QString &directory, QStringList &associations,
    std::unordered_set<QString> &denyList)
{
    static auto desktopNames = qEnvironmentVariable("XDG_CURRENT_DESKTOP")
                                   .split(':', Qt::SkipEmptyParts);
    static const QString desktopFilename = QStringLiteral("%1-mimeapps.list");
    static const QString nonDesktopFilename = QStringLiteral("mimeapps.list");

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

/// Try to figure out the most reasonably default web browser to use
///
/// If the `xdg-settings` program is available, use that
/// If not, read through all possible mimapps files in the order specified here: https://specifications.freedesktop.org/mime-apps-spec/mime-apps-spec-1.0.1.html#file
/// If no mimeapps file has a default, try to use the Added Associations in those files
std::optional<XDGDesktopFile> getDefaultBrowserDesktopFile()
{
    // no xdg-utils, find it manually by searching mimeapps.list files
    QStringList associations;
    std::unordered_set<QString> denyList;

    // config dirs first
    for (const auto &configDir : getXDGDirectories(XDGDirectoryType::Config))
    {
        auto defaultApp =
            searchMimeAppsListsInDirectory(configDir, associations, denyList);
        if (defaultApp.has_value())
        {
            return defaultApp;
        }
    }

    // data dirs for backwards compatibility
    for (const auto &dataDir : getXDGDirectories(XDGDirectoryType::Data))
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
            auto desktopFile = XDGDesktopFile::findDesktopFile(desktopId);
            if (desktopFile.has_value())
            {
                return desktopFile;
            }
        }
    }

    // use xdg-settings if installed
    QProcess xdgSettings;
    xdgSettings.start("xdg-settings", {"get", "default-web-browser"},
                      QIODevice::ReadOnly);
    xdgSettings.waitForFinished(1000);
    if (xdgSettings.exitStatus() == QProcess::ExitStatus::NormalExit &&
        xdgSettings.error() == QProcess::UnknownError &&
        xdgSettings.exitCode() == 0)
    {
        return XDGDesktopFile::findDesktopFile(
            xdgSettings.readAllStandardOutput().trimmed());
    }

    return {};
}

QString parseDesktopExecProgram(const QString &execKey)
{
    static const QRegularExpression unescapeReservedCharacters(
        R"(\\(["`$\\]))");

    QString program = execKey;

    // string values in desktop files escape all backslashes. This is an
    // independent escaping scheme that must be processed first
    program.replace(u"\\\\"_s, u"\\"_s);

    if (!program.startsWith('"'))
    {
        // not quoted, trim after the first space (if any)
        auto end = program.indexOf(' ');
        if (end != -1)
        {
            program = program.left(end);
        }
    }
    else
    {
        // quoted
        auto endQuote = program.indexOf('"', 1);
        if (endQuote == -1)
        {
            // No end quote found, the returned program might be malformed
            program = program.mid(1);
            qCWarning(LOG).noquote().nospace()
                << "Malformed desktop entry key " << program << ", originally "
                << execKey << ", you might run into issues";
        }
        else
        {
            // End quote found
            program = program.mid(1, endQuote - 1);
        }
    }

    // program now contains the first token of the command line.
    // this is either the program name with an absolute path, or just the program name
    // denoting it's a relative path. Either will be handled by QProcess cleanly
    // now, there is a second escaping scheme specific to the
    // exec key that must be applied.
    program.replace(unescapeReservedCharacters, "\\1");

    return program;
}

}  // namespace chatterino

#endif
