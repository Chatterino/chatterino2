#pragma once

#include "QStringHash.hpp"

#include <optional>
#include <unordered_map>

#if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)

namespace chatterino {

class XDGDesktopFile
{
public:
    explicit XDGDesktopFile(const QString &filename);
    XDGDesktopFile(XDGDesktopFile &&other) = default;
    ~XDGDesktopFile() = default;

    using Group = std::unordered_map<QString, QString>;
    Group const &operator[](QString const &key) const;

    bool exists() const
    {
        return _exists;
    }

    static std::optional<XDGDesktopFile> findDesktopId(
        QString const &desktopId);

private:
    bool _exists = false;
    std::unordered_map<QString, Group> _groups;
};

}  // namespace chatterino

#endif
