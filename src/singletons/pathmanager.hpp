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

    // Logs
    QString logsFolderPath;
    QString channelsLogsFolderPath;
    QString whispersLogsFolderPath;
    QString mentionsLogsFolderPath;

    QString appPathHash;

    bool createFolder(const QString &folderPath);
};

}  // namespace singletons
}  // namespace chatterino
