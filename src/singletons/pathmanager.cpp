#include "singletons/pathmanager.hpp"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>
#include <cassert>

namespace chatterino {
namespace singletons {

PathManager *PathManager::instance = nullptr;

PathManager::PathManager(int argc, char **argv)
{
    // hash of app path
    this->appPathHash = QCryptographicHash::hash(QCoreApplication::applicationFilePath().toUtf8(),
                                                 QCryptographicHash::Sha224)
                            .toBase64()
                            .mid(0, 32)
                            .replace("+", "-")
                            .replace("/", "x");

    // Options
    this->portable = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "portable") == 0) {
            this->portable = true;
        }
    }

    if (QFileInfo::exists(QCoreApplication::applicationDirPath() + "/this->portable")) {
        this->portable = true;
    }

    // Root path = %APPDATA%/Chatterino or the folder that the executable resides in
    QString rootPath;
    if (this->portable) {
        rootPath.append(QCoreApplication::applicationDirPath());
    } else {
        // Get settings path
        rootPath.append(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        if (rootPath.isEmpty()) {
            throw std::runtime_error("Error finding writable location for settings");
        }
    }

    this->settingsFolderPath = rootPath;

    if (!QDir().mkpath(this->settingsFolderPath)) {
        throw std::runtime_error("Error creating settings folder");
    }

    this->customFolderPath = rootPath + "/Custom";

    if (!QDir().mkpath(this->customFolderPath)) {
        throw std::runtime_error("Error creating custom folder");
    }

    this->cacheFolderPath = rootPath + "/Cache";

    if (!QDir().mkpath(this->cacheFolderPath)) {
        throw std::runtime_error("Error creating cache folder");
    }

    this->logsFolderPath = rootPath + "/Logs";

    if (!QDir().mkpath(this->logsFolderPath)) {
        throw std::runtime_error("Error creating logs folder");
    }
}

void PathManager::initInstance(int argc, char **argv)
{
    assert(!instance);

    instance = new PathManager(argc, argv);
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
    return this->portable;
}

}  // namespace singletons
}  // namespace chatterino
