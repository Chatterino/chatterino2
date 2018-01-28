#pragma once

#include <QString>

namespace chatterino {
namespace singletons {

class PathManager
{
    PathManager() = default;

public:
    static PathManager &getInstance();

    bool init(int argc, char **argv);

    QString settingsFolderPath;
    QString customFolderPath;
    QString cacheFolderPath;

    // Logs
    QString logsFolderPath;
    QString channelsLogsFolderPath;
    QString whispersLogsFolderPath;
    QString mentionsLogsFolderPath;

    bool createFolder(const QString &folderPath);
};

}  // namespace singletons
}  // namespace chatterino
