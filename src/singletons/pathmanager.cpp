#include "singletons/pathmanager.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace chatterino {
namespace singletons {

PathManager &PathManager::getInstance()
{
    static PathManager instance;
    return instance;
}

bool PathManager::init(int argc, char **argv)
{
    // Options
    bool portable = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "portable") == 0) {
            portable = true;
        }
    }

    if (QFileInfo::exists(QCoreApplication::applicationDirPath() + "/portable")) {
        portable = true;
    }

    // Root path = %APPDATA%/Chatterino or the folder that the executable resides in
    QString rootPath;
    if (portable) {
        rootPath.append(QCoreApplication::applicationDirPath());
    } else {
        // Get settings path
        rootPath.append(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        if (rootPath.isEmpty()) {
            printf("Error finding writable location for settings\n");
            return false;
        }
    }

    this->settingsFolderPath = rootPath;

    if (!QDir().mkpath(this->settingsFolderPath)) {
        printf("Error creating directory: %s\n", qPrintable(this->settingsFolderPath));
        return false;
    }

    this->customFolderPath = rootPath + "/Custom";

    if (!QDir().mkpath(this->customFolderPath)) {
        printf("Error creating directory: %s\n", qPrintable(this->customFolderPath));
        return false;
    }

    this->cacheFolderPath = rootPath + "/Cache";

    if (!QDir().mkpath(this->cacheFolderPath)) {
        printf("Error creating cache directory: %s\n", qPrintable(this->cacheFolderPath));
        return false;
    }

    this->logsFolderPath = rootPath + "/Logs";

    if (!QDir().mkpath(this->logsFolderPath)) {
        printf("Error creating logs directory: %s\n", qPrintable(this->logsFolderPath));
        return false;
    }

    this->channelsLogsFolderPath = this->logsFolderPath + "/Channels";

    if (!QDir().mkpath(this->channelsLogsFolderPath)) {
        printf("Error creating channelsLogs directory: %s\n",
               qPrintable(this->channelsLogsFolderPath));
        return false;
    }

    this->whispersLogsFolderPath = this->logsFolderPath + "/Whispers";

    if (!QDir().mkpath(this->whispersLogsFolderPath)) {
        printf("Error creating whispersLogs directory: %s\n",
               qPrintable(this->whispersLogsFolderPath));
        return false;
    }

    this->mentionsLogsFolderPath = this->logsFolderPath + "/Mentions";

    if (!QDir().mkpath(this->mentionsLogsFolderPath)) {
        printf("Error creating mentionsLogs directory: %s\n",
               qPrintable(this->mentionsLogsFolderPath));
        return false;
    }

    return true;
}

bool PathManager::createFolder(const QString &folderPath)
{
    return QDir().mkpath(folderPath);
}

}  // namespace singletons
}  // namespace chatterino
