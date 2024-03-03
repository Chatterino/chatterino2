#include "singletons/Paths.hpp"

#include "common/Modes.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

#include <cassert>

using namespace std::literals;

namespace chatterino {

Paths::Paths()
{
    this->initAppFilePathHash();

    this->initCheckPortable();
    this->initRootDirectory();
    this->initSubDirectories();
}

bool Paths::createFolder(const QString &folderPath)
{
    return QDir().mkpath(folderPath);
}

bool Paths::isPortable() const
{
    return Modes::instance().isPortable;
}

QString Paths::cacheDirectory() const
{
    static const auto pathSetting = [] {
        QStringSetting cachePathSetting("/cache/path");

        cachePathSetting.connect([](const auto &newPath, auto) {
            if (!newPath.isEmpty())
            {
                QDir().mkpath(newPath);
            }
        });

        return cachePathSetting;
    }();

    auto path = pathSetting.getValue();

    if (path.isEmpty())
    {
        return this->cacheDirectory_;
    }

    return path;
}

void Paths::initAppFilePathHash()
{
    this->applicationFilePathHash =
        QCryptographicHash::hash(
            QCoreApplication::applicationFilePath().toUtf8(),
            QCryptographicHash::Sha224)
            .toBase64()
            .mid(0, 32)
            .replace("+", "-")
            .replace("/", "x");
}

void Paths::initCheckPortable()
{
    this->portable_ = QFileInfo::exists(
        combinePath(QCoreApplication::applicationDirPath(), "portable"));
}

void Paths::initRootDirectory()
{
    assert(this->portable_.has_value());

    // Root path = %APPDATA%/Chatterino or the folder that the executable
    // resides in

    this->rootAppDataDirectory = [&]() -> QString {
        // portable
        if (Modes::instance().isPortable)
        {
            return QCoreApplication::applicationDirPath();
        }

        // permanent installation
        QString path =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (path.isEmpty())
        {
            throw std::runtime_error("Could not create directory \""s +
                                     path.toStdString() + "\"");
        }

// create directory Chatterino2 instead of Chatterino on windows because the
// ladder one is takes by Chatterino 1 already
#ifdef Q_OS_WIN
        path.replace("chatterino", "Chatterino");

        path += "2";
#endif
        return path;
    }();
}

void Paths::initSubDirectories()
{
    // required the app data directory to be set first
    assert(!this->rootAppDataDirectory.isEmpty());

    // create settings subdirectories and validate that they are created
    // properly
    auto makePath = [&](const QString &name) -> QString {
        auto path = combinePath(this->rootAppDataDirectory, name);

        if (!QDir().mkpath(path))
        {
            throw std::runtime_error("Could not create directory \""s +
                                     path.toStdString() + "\"");
        }

        return path;
    };

    makePath("");
    this->settingsDirectory = makePath("Settings");
    this->cacheDirectory_ = makePath("Cache");
    this->messageLogDirectory = makePath("Logs");
    this->miscDirectory = makePath("Misc");
    this->twitchProfileAvatars =
        makePath(combinePath("ProfileAvatars", "twitch"));
    this->pluginsDirectory = makePath("Plugins");
    this->themesDirectory = makePath("Themes");
    this->crashdumpDirectory = makePath("Crashes");
#ifdef Q_OS_WIN
    this->ipcDirectory = makePath("IPC");
#else
    // NOTE: We do *NOT* use IPC on non-Windows platforms.
    // If we start, we should re-consider this directory.
    this->ipcDirectory = "/tmp";
#endif
}

}  // namespace chatterino
