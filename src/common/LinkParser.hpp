#pragma once

#include <QString>

#include <optional>

namespace chatterino {

struct ParsedLink {
    /// The parsed protocol of the link. Can be empty.
    ///
    /// https://www.forsen.tv/commands
    /// ^------^
    QStringView protocol;

    /// The parsed host of the link. Can not be empty.
    ///
    /// https://www.forsen.tv/commands
    ///         ^-----------^
    QStringView host;

    /// The remainder of the link. Can be empty.
    ///
    /// https://www.forsen.tv/commands
    ///                      ^-------^
    QStringView rest;

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
