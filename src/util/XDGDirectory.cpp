#include "util/XDGDirectory.hpp"

#include "util/CombinePath.hpp"
#include "util/Qt.hpp"

namespace chatterino {

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

QStringList getXDGDirectories(XDGDirectoryType directory)
{
    static std::unordered_map<XDGDirectoryType,
                              std::pair<const char *, QString>>
        userDirectories = {
            {XDGDirectoryType::Config,
             {"XDG_CONFIG_HOME", combinePath(QDir::homePath(), ".config/")}},
            {XDGDirectoryType::Data,
             {"XDG_DATA_HOME",
              combinePath(QDir::homePath(), ".local/share/")}}};

    auto const &[userEnvVar, userDefaultValue] = userDirectories.at(directory);
    auto userEnvPath = qEnvironmentVariable(userEnvVar);
    QStringList paths{userEnvPath.isEmpty() ? userDefaultValue : userEnvPath};

    static std::unordered_map<XDGDirectoryType,
                              std::pair<const char *, QStringList>>
        baseDirectories = {
            {XDGDirectoryType::Config, {"XDG_CONFIG_DIRS", {"/etc/xdg"}}},
            {XDGDirectoryType::Data,
             {"XDG_DATA_DIRS", {"/usr/local/share/", "/usr/share/"}}}};

    const auto &[baseEnvVar, baseDefaultValue] = baseDirectories.at(directory);
    auto baseEnvPaths =
        qEnvironmentVariable(baseEnvVar).split(':', Qt::SkipEmptyParts);
    paths << (baseEnvPaths.isEmpty() ? baseDefaultValue : baseEnvPaths);

    return paths;
}

#endif

}  // namespace chatterino
