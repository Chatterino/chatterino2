#pragma once

#include <QString>

namespace chatterino {
namespace singletons {

class PathManager
{
public:
    PathManager(int argc, char **argv);

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
    bool portable;
};

}  // namespace singletons
}  // namespace chatterino
