#pragma once

#include <QFileDevice>
#include <QString>

#include <filesystem>

namespace chatterino {

inline QString stdPathToQString(const std::filesystem::path &path)
{
#ifdef Q_OS_WIN
    return QString::fromStdWString(path.native());
#else
    return QString::fromStdString(path.native());
#endif
}

inline std::filesystem::path qStringToStdPath(const QString &path)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto *ptr = reinterpret_cast<const char16_t *>(path.utf16());
    return {ptr, ptr + path.size()};
}

}  // namespace chatterino
