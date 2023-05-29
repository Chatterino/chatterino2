#pragma once

#include <boost/optional.hpp>
#include <QString>

namespace chatterino {

class Paths
{
public:
    static Paths *instance;

    Paths();

    // Root directory for the configuration files. %APPDATA%/chatterino or
    // ExecutablePath for portable mode
    QString rootAppDataDirectory;

    // Directory for settings files. Same as <appDataDirectory>/Settings
    QString settingsDirectory;

    // Directory for message log files. Same as <appDataDirectory>/Misc
    QString messageLogDirectory;

    // Directory for miscellaneous files. Same as <appDataDirectory>/Misc
    QString miscDirectory;

    // Directory for crashdumps. Same as <appDataDirectory>/Crashes
    QString crashdumpDirectory;

    // Hash of QCoreApplication::applicationFilePath()
    QString applicationFilePathHash;

    // Profile avatars for Twitch <appDataDirectory>/cache/twitch
    QString twitchProfileAvatars;

    // Plugin files live here. <appDataDirectory>/Plugins
    QString pluginsDirectory;

    // Custom themes live here. <appDataDirectory>/Themes
    QString themesDirectory;

    bool createFolder(const QString &folderPath);
    bool isPortable();

    QString cacheDirectory();

private:
    void initAppFilePathHash();
    void initCheckPortable();
    void initRootDirectory();
    void initSubDirectories();

    boost::optional<bool> portable_;

    // Directory for cache files. Same as <appDataDirectory>/Misc
    QString cacheDirectory_;
};

Paths *getPaths();

}  // namespace chatterino
