#pragma once

#include <QString>
#include <boost/optional.hpp>

namespace chatterino {

class Paths
{
public:
    static Paths *instance;

    Paths();

    // Root directory for the configuration files. %APPDATA%/chatterino or ExecutablePath for
    // portable mode
    QString rootAppDataDirectory;

    // Directory for settings files. Same as <appDataDirectory>/Settings
    QString settingsDirectory;

    // Directory for cache files. Same as <appDataDirectory>/Misc
    QString cacheDirectory;

    // Directory for message log files. Same as <appDataDirectory>/Misc
    QString messageLogDirectory;

    // Directory for miscellaneous files. Same as <appDataDirectory>/Misc
    QString miscDirectory;

    // Hash of QCoreApplication::applicationFilePath()
    QString applicationFilePathHash;

    bool createFolder(const QString &folderPath);
    bool isPortable();

private:
    void initAppFilePathHash();
    void initCheckPortable();
    void initAppDataDirectory();
    void initSubDirectories();

    boost::optional<bool> portable_;
};

[[deprecated]] Paths *getPaths();

}  // namespace chatterino
