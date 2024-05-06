#pragma once

#include <QString>

#include <optional>

namespace chatterino {

struct ParsedLink {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    using StringView = QStringView;
#else
    using StringView = QStringRef;
#endif
    /// The parsed protocol of the link. Can be empty.
    ///
    /// https://www.forsen.tv/commands
    /// ^------^
    StringView protocol;

    /// The parsed host of the link. Can not be empty.
    ///
    /// https://www.forsen.tv/commands
    ///         ^-----------^
    StringView host;

    /// The remainder of the link. Can be empty.
    ///
    /// https://www.forsen.tv/commands
    ///                      ^-------^
    StringView rest;

    /// The original unparsed link.
    ///
    /// https://www.forsen.tv/commands
    /// ^----------------------------^
    QString source;
};

class LinkParser
{
public:
    explicit LinkParser(const QString &unparsedString);

    const std::optional<ParsedLink> &result() const;

private:
    std::optional<ParsedLink> result_{};
};

}  // namespace chatterino
