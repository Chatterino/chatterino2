#include "util/XDGDirectory.hpp"

#include "util/CombinePath.hpp"

#include <unordered_map>

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

QStringList getXDGBaseDirectories(XDGDirectoryType directory)
{
    // Base (or system) XDG directory environment variables with defaults
    // Defaults fetched from https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html#variables 2023-08-05
    static std::unordered_map<XDGDirectoryType,
                              std::pair<const char *, QStringList>>
        baseDirectories = {
            {
                XDGDirectoryType::Config,
                {
                    "XDG_CONFIG_DIRS",
                    {"/etc/xdg"},
                },
            },
            {
                XDGDirectoryType::Data,
                {
                    "XDG_DATA_DIRS",
                    {"/usr/local/share/", "/usr/share/"},
                },
            },
        };

    QStringList paths;

    const auto &[baseEnvVar, baseDefaultValue] = baseDirectories.at(directory);
    auto baseEnvPaths =
        qEnvironmentVariable(baseEnvVar).split(':', Qt::SkipEmptyParts);
    if (baseEnvPaths.isEmpty())
    {
        paths.append(baseDefaultValue);
    }
    else
    {
        paths.append(baseEnvPaths);
    }

    return paths;
}

QStringList getXDGUserDirectories(XDGDirectoryType directory)
{
    // User XDG directory environment variables with defaults
    // Defaults fetched from https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html#variables 2023-08-05
    static std::unordered_map<XDGDirectoryType,
                              std::pair<const char *, QString>>
        userDirectories = {
            {
                XDGDirectoryType::Config,
                {
                    "XDG_CONFIG_HOME",
                    combinePath(QDir::homePath(), ".config/"),
                },
            },
            {
                XDGDirectoryType::Data,
                {
                    "XDG_DATA_HOME",
                    combinePath(QDir::homePath(), ".local/share/"),
                },
            },
        };

    const auto &[userEnvVar, userDefaultValue] = userDirectories.at(directory);
    return {
        qEnvironmentVariable(userEnvVar, userDefaultValue),
    };
}

QStringList getXDGDirectories(XDGDirectoryType directory)
{
    return getXDGUserDirectories(directory) + getXDGBaseDirectories(directory);
}

#endif

}  // namespace chatterino
