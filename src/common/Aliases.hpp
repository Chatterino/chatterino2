// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <boost/container_hash/hash_fwd.hpp>
#include <QHashFunctions>
#include <QString>

#include <cstddef>
#include <functional>

//! This file defines strong aliases for string types.
//!
//! Each alias comes with a `Name` and `NameView` where `Name` wraps a `QString`
//! and `NameView` wraps a `QStringView`. Furthermore, `NameHash` is a
//! transparent hasher for use in unordered containers to allow lookup by both
//! `Name` and `NameView`.
//!
//! Prefer the view type wherever the value isn't stored (e.g. in a map lookup).
//! To get the full benefit for containers, use the transparent comparators
//! `std::equal_to<>` and `std::less<>` - they allow lookup with the view type:
//!
//! - `std::map<Name, V, std::less<>>`
//! - `std::unordered_map<Name, V, NameHash, std::equal_to<>>`
//! - `boost::unordered_flat_map<Name, V, NameHash, std::equal_to<>>`
//!
//! Instead of `NameHash` it is also okay to use `std::hash<Name>` or
//! `boost::hash<Name>` - all are transparent.

// TODO(Qt6.8+): Use operator<=>(lhs, rhs) = default.

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define QStringAliasCompareOp(name, OP)                                \
    friend bool operator OP(const name &lhs, const name &rhs) noexcept \
    {                                                                  \
        return lhs.string OP rhs.string;                               \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define QStringAliasCompare(name)    \
    QStringAliasCompareOp(name, ==); \
    QStringAliasCompareOp(name, !=); \
    QStringAliasCompareOp(name, <);  \
    QStringAliasCompareOp(name, >);  \
    QStringAliasCompareOp(name, <=); \
    QStringAliasCompareOp(name, >=);

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define QStringAlias(name)                                         \
    namespace chatterino {                                         \
    struct name {                                                  \
        QString string;                                            \
        QStringAliasCompare(name);                                 \
    };                                                             \
    struct name##View {                                            \
        QStringView string;                                        \
        explicit constexpr name##View(QStringView s)               \
            : string(s)                                            \
        {                                                          \
        }                                                          \
        name##View(const name &n)                                  \
            : string(n.string)                                     \
        {                                                          \
        }                                                          \
        [[nodiscard]] name toOwned() const                         \
        {                                                          \
            return {this->string.toString()};                      \
        }                                                          \
        QStringAliasCompare(name##View);                           \
    };                                                             \
    struct name##Hash {                                            \
        using is_transparent = void;                               \
        /* Only allows NameView and Name to be hashed. */          \
        size_t operator()(name##View n) const                      \
        {                                                          \
            return qHash(n.string);                                \
        }                                                          \
    };                                                             \
    } /* namespace chatterino */                                   \
    namespace std {                                                \
    template <>                                                    \
    struct hash<chatterino::name> : chatterino::name##Hash {       \
    };                                                             \
    template <>                                                    \
    struct hash<chatterino::name##View> : chatterino::name##Hash { \
    };                                                             \
    } /* namespace std */                                          \
    namespace boost {                                              \
    template <>                                                    \
    struct hash<chatterino::name> : chatterino::name##Hash {       \
    };                                                             \
    template <>                                                    \
    struct hash<chatterino::name##View> : chatterino::name##Hash { \
    };                                                             \
    } /* namespace boost */

QStringAlias(UserName);
QStringAlias(UserId);
QStringAlias(Url);
QStringAlias(Tooltip);
QStringAlias(EmoteId);
QStringAlias(EmoteSetId);
QStringAlias(EmoteName);
QStringAlias(EmoteAuthor);

#undef QStringAlias
#undef QStringAliasCompare
#undef QStringAliasCompareOp
