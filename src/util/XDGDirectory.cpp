#include "util/XDGDirectory.hpp"

#include "util/CombinePath.hpp"
#include "util/Qt.hpp"

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

QStringList getXDGDirectories(XDGDirectoryType directory)
{
    // User XDG directory environment variables with defaults
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

    // Base (or system) XDG directory environment variables with defaults
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

    auto const &[userEnvVar, userDefaultValue] = userDirectories.at(directory);
    auto userEnvPath = qEnvironmentVariable(userEnvVar, userDefaultValue);
    paths.push_back(userEnvPath);

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

#endif

}  // namespace chatterino
