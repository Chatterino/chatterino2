#include "singletons/Paths.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <cassert>

#include "util/CombinePath.hpp"

namespace chatterino {

PathManager *PathManager::instance = nullptr;

PathManager::PathManager()
{
    this->initAppFilePathHash();

    this->initCheckPortable();
    this->initAppDataDirectory();
    this->initSubDirectories();
}

void PathManager::initInstance()
{
    assert(!instance);

    instance = new PathManager();
}

PathManager *PathManager::getInstance()
{
    assert(instance);

    return instance;
}

bool PathManager::createFolder(const QString &folderPath)
{
    return QDir().mkpath(folderPath);
}

bool PathManager::isPortable()
{
    return this->portable.get();
}

void PathManager::initAppFilePathHash()
{
    this->applicationFilePathHash =
        QCryptographicHash::hash(QCoreApplication::applicationFilePath().toUtf8(),
                                 QCryptographicHash::Sha224)
            .toBase64()
            .mid(0, 32)
            .replace("+", "-")
            .replace("/", "x");
}

void PathManager::initCheckPortable()
{
    this->portable =
        QFileInfo::exists(combinePath(QCoreApplication::applicationDirPath(), "portable"));
}

void PathManager::initAppDataDirectory()
{
    assert(this->portable.is_initialized());

    // Root path = %APPDATA%/Chatterino or the folder that the executable resides in

    this->rootAppDataDirectory = [&]() -> QString {
        // portable
        if (this->isPortable()) {
            return QCoreApplication::applicationDirPath();
        }

        // permanent installation
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (path.isEmpty()) {
            throw std::runtime_error("Error finding writable location for settings");
        }

// create directory Chatterino2 instead of chatterino on windows because the ladder one is takes by
// chatterino 1 already
#ifdef Q_OS_WIN
        path.replace("chatterino", "Chatterino");

        path += "2";
#endif
        return path;
    }();
}

void PathManager::initSubDirectories()
{
    // required the app data directory to be set first
    assert(!this->rootAppDataDirectory.isEmpty());

    // create settings subdirectories and validate that they are created properly
    auto makePath = [&](const std::string &name) -> QString {

        auto path = combinePath(this->rootAppDataDirectory, QString::fromStdString(name));

        if (!QDir().mkpath(path)) {
            throw std::runtime_error("Error creating appdata path %appdata%/chatterino/" + name);
        }

        return path;
    };

    makePath("");
    this->settingsDirectory = makePath("Settings");
    this->cacheDirectory = makePath("Cache");
    this->messageLogDirectory = makePath("Logs");
    this->miscDirectory = makePath("Misc");
}

}  // namespace chatterino
