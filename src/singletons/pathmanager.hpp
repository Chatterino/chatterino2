#pragma once

#include <QString>

namespace chatterino {
namespace singletons {

class PathManager
{
    PathManager(int argc, char **argv);

public:
    static void initInstance(int argc, char **argv);
    static PathManager *getInstance();

    // %APPDATA%/chatterino or ExecutablePath for portable mode
    QString settingsFolderPath;

    // %APPDATA%/chatterino/Custom or ExecutablePath/Custom for portable mode
    QString customFolderPath;

    // %APPDATA%/chatterino/Cache or ExecutablePath/Cache for portable mode
    QString cacheFolderPath;

    // Default folder for logs. %APPDATA%/chatterino/Logs or ExecutablePath/Logs for portable mode
    QString logsFolderPath;

    QString appPathHash;

    bool createFolder(const QString &folderPath);
    bool isPortable();

private:
    static PathManager *instance;
    bool portable;
};

}  // namespace singletons
}  // namespace chatterino
