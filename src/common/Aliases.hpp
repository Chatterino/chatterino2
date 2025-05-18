#pragma once

#include <boost/container_hash/hash_fwd.hpp>
#include <QHashFunctions>
#include <QString>

#include <cstddef>
#include <functional>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define QStringAlias(name)                                      \
    namespace chatterino {                                      \
    struct name {                                               \
        QString string;                                         \
        bool operator==(const name &other) const                \
        {                                                       \
            return this->string == other.string;                \
        }                                                       \
        bool operator!=(const name &other) const                \
        {                                                       \
            return this->string != other.string;                \
        }                                                       \
    };                                                          \
    } /* namespace chatterino */                                \
    namespace std {                                             \
    template <>                                                 \
    struct hash<chatterino::name> {                             \
        size_t operator()(const chatterino::name &s) const      \
        {                                                       \
            return qHash(s.string);                             \
        }                                                       \
    };                                                          \
    } /* namespace std */                                       \
    namespace boost {                                           \
    template <>                                                 \
    struct hash<chatterino::name> {                             \
        std::size_t operator()(chatterino::name const &s) const \
        {                                                       \
            return qHash(s.string);                             \
        }                                                       \
    };                                                          \
    } /* namespace boost */

QStringAlias(UserName);
QStringAlias(UserId);
QStringAlias(Url);
QStringAlias(Tooltip);
QStringAlias(EmoteId);
QStringAlias(EmoteSetId);
QStringAlias(EmoteName);
QStringAlias(EmoteAuthor);
