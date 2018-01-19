#include "pathmanager.hpp"

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

    // Root path = %APPDATA%/Chatterino or the folder that the executable resides in
    QString rootPath;
    if (portable) {
        rootPath.append(QDir::currentPath());
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

    return true;
}

}  // namespace singletons
}  // namespace chatterino
