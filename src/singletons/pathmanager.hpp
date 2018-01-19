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
};

}  // namespace singletons
}  // namespace chatterino
