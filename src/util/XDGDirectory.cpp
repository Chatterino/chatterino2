#include "util/XDGDirectory.hpp"

#include "util/CombinePath.hpp"
#include "util/Qt.hpp"

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace chatterino {

QStringList splitEnvironmentVariable(const char *name)
{
    return qEnvironmentVariable(name).split(':', Qt::SkipEmptyParts);
}

QStringList getXDGDirectories(XDGDirectory directory)
{
    static std::unordered_map<XDGDirectory, std::pair<const char *, QString>>
        userDirectories = {
            {XDGDirectory::Config,
             {"XDG_CONFIG_HOME", combinePath(QDir::homePath(), ".config/")}},
            {XDGDirectory::Data,
             {"XDG_DATA_HOME",
              combinePath(QDir::homePath(), ".local/share/")}}};

    auto const &[userEnvVar, userDefaultValue] = userDirectories.at(directory);
    auto userEnvPath = qEnvironmentVariable(userEnvVar);
    QStringList paths{userEnvPath.isEmpty() ? userDefaultValue : userEnvPath};

    static std::unordered_map<XDGDirectory,
                              std::pair<const char *, QStringList>>
        baseDirectories = {
            {XDGDirectory::Config, {"XDG_CONFIG_DIRS", {"/etc/xdg"}}},
            {XDGDirectory::Data,
             {"XDG_DATA_DIRS", {"/usr/local/share/", "/usr/share/"}}}};

    const auto &[baseEnvVar, baseDefaultValue] = baseDirectories.at(directory);
    auto baseEnvPaths = splitEnvironmentVariable(baseEnvVar);
    paths << (baseEnvPaths.isEmpty() ? baseDefaultValue : baseEnvPaths);

    return paths;
}

}  // namespace chatterino

#endif
