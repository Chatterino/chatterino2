#pragma once

#include <QString>

#include <optional>

namespace chatterino::linkparser {

/// @brief Represents a parsed link
///
/// A parsed link is represented as views over the source string for its
/// different segments. In this simplified model, a link consists of an optional
/// @a protocol, a mandatory @a host and an optional @a rest. These segments are
/// always next to eachother in the input string, however together, they don't
/// span the whole input as it could contain prefixes or suffixes.
///
/// Prefixes and suffixes are almost identical to the ones in GitHub Flavored
/// Markdown (GFM - https://github.github.com/gfm/#autolinks-extension-).
/// The main difference is that '_' isn't a valid suffix.
/// Parentheses are counted inside the @a rest: parsing "(a.com/(foo))" would
/// result in the link "a.com/(foo)".
/// Matching is done case insensitive (e.g. "HTTp://a.com" would be valid).
///
/// A @a protocol can either be empty, "http://", or "https://".
/// A @a host can either be an IPv4 address or a hostname. The hostname must end
/// in a valid top level domain. Otherwise, there are no restrictions on it.
/// The @a rest can start with an optional port followed by either a '/', '?',
/// or '#'.
///
/// @b Example
///
/// ```text
/// (https://wiki.chatterino.com/Help/#overview)
/// ▏▏proto ▕       host        ▏     rest     ▏▏
/// ▏▏                link                     ▏▏
/// ▏                source                     ▏
/// ```
struct Parsed {
    /// The parsed protocol of the link. Can be empty.
    ///
    /// ```text
    /// https://www.forsen.tv/commands
    /// ▏╌╌╌╌╌╌▕
    ///
    /// www.forsen.tv/commands
    /// (empty)
    /// ```
    QStringView protocol;

    /// The parsed host of the link. Can not be empty.
    ///
    /// ```text
    /// https://www.forsen.tv/commands
    ///         ▏╌╌╌╌╌╌╌╌╌╌╌▕
    /// ```
    QStringView host;

    /// The remainder of the link. Can be empty.
    ///
    /// ```text
    /// https://www.forsen.tv/commands
    ///                      ▏╌╌╌╌╌╌╌▕
    ///
    /// https://www.forsen.tv
    /// (empty)
    /// ```
    QStringView rest;

    /// The matched link. Can not be empty.
    ///
    /// ```text
    /// (https://www.forsen.tv/commands)
    ///  ▏╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌▕
    /// ```
    QStringView link;

    /// Checks if the parsed link contains a prefix
    bool hasPrefix(const QString &source) const noexcept
    {
        return this->link.begin() != source.begin();
    }

    /// The prefix before the parsed link inside @a source. May be empty.
    ///
    /// ```text
    /// (https://www.forsen.tv/commands)
    /// ^
    ///
    /// https://www.forsen.tv/commands
    /// (empty)
    /// ```
    QStringView prefix(const QString &source) const noexcept
    {
        return {source.data(), this->link.begin()};
    }

    /// Checks if the parsed link contains a suffix
    bool hasSuffix(const QString &source) const noexcept
    {
        return this->link.end() != source.end();
    }

    /// The suffix after the parsed link inside @a source. May be empty.
    ///
    /// ```text
    /// (https://www.forsen.tv/commands)
    ///                                ^
    ///
    /// https://www.forsen.tv/commands
    /// (empty)
    /// ```
    QStringView suffix(const QString &source) const noexcept
    {
        return {
            this->link.begin() + this->link.size(),
            source.data() + source.length(),
        };
    }
};

/// @brief Parses a link from @a source into its segments
///
/// If no link is contained in @a source, `std::nullopt` will be returned.
/// The returned value is valid as long as @a source exists, as it contains
/// views into @a source.
///
/// For the accepted links, see Parsed.
std::optional<Parsed> parse(const QString &source) noexcept;

}  // namespace chatterino::linkparser
