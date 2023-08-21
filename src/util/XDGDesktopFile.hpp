#pragma once

#include "util/QStringHash.hpp"

#include <optional>
#include <unordered_map>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace chatterino {

// See https://specifications.freedesktop.org/desktop-entry-spec/desktop-entry-spec-latest.html#group-header
using XDGEntries = std::unordered_map<QString, QString>;

class XDGDesktopFile
{
public:
    // Read the file at `filename` as an XDG desktop file, parsing its groups & their entries
    //
    // Use the `isValid` function to check if the file was read properly
    explicit XDGDesktopFile(const QString &filename);

    /// Returns a map of entries for the given group header
    XDGEntries getEntries(const QString &groupHeader) const;

    /// isValid returns true if the file exists and is readable
    bool isValid() const
    {
        return valid;
    }

    /// Find the first desktop file based on the given desktop file ID
    ///
    /// This will look through all Data XDG directories
    ///
    /// Can return std::nullopt if no desktop file was found for the given desktop file ID
    ///
    /// References: https://specifications.freedesktop.org/desktop-entry-spec/latest/ar01s02.html#desktop-file-id
    static std::optional<XDGDesktopFile> findDesktopFile(
        const QString &desktopFileID);

private:
    bool valid{};
    std::unordered_map<QString, XDGEntries> groups;
};

}  // namespace chatterino

#endif
