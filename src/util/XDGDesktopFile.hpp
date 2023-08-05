#pragma once

#include "QStringHash.hpp"

#include <optional>
#include <unordered_map>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace chatterino {

// See https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#group-header
using XDGEntries = std::unordered_map<QString, QString>;

class XDGDesktopFile
{
public:
    explicit XDGDesktopFile(const QString &filename);
    XDGDesktopFile(XDGDesktopFile &&other) = default;
    ~XDGDesktopFile() = default;

    /// Returns a map of entries for the given group header
    XDGEntries getEntries(const QString &groupHeader) const;

    bool exists() const
    {
        return _exists;
    }

    static std::optional<XDGDesktopFile> findDesktopId(
        QString const &desktopId);

private:
    bool _exists = false;
    std::unordered_map<QString, XDGEntries> _groups;
};

}  // namespace chatterino

#endif
