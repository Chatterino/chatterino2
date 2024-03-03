#pragma once

#include <QString>

#include <optional>

namespace chatterino {

class Paths
{
public:
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

    // Profile avatars for Twitch <appDataDirectory>/ProfileAvatars/twitch
    QString twitchProfileAvatars;

    // Plugin files live here. <appDataDirectory>/Plugins
    QString pluginsDirectory;

    // Custom themes live here. <appDataDirectory>/Themes
    QString themesDirectory;

    // Directory for shared memory files.
    // <appDataDirectory>/IPC   on Windows
    // /tmp                     elsewhere
    QString ipcDirectory;

    bool createFolder(const QString &folderPath);
    [[deprecated("use Modes::instance().portable instead")]] bool isPortable()
        const;

    QString cacheDirectory() const;

private:
    void initAppFilePathHash();
    void initCheckPortable();
    void initRootDirectory();
    void initSubDirectories();

    std::optional<bool> portable_;

    // Directory for cache files. Same as <appDataDirectory>/Misc
    QString cacheDirectory_;
};

}  // namespace chatterino
