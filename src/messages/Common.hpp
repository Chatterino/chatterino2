#pragma once

#include "util/Outcome.hpp"

#include <QHash>
#include <QString>
#include <QWidget>
#include <boost/optional.hpp>
#include <boost/preprocessor.hpp>
#include <functional>
#include <string>

namespace chatterino
{
    /// ENUMS
    enum class HighlightState {
        None,
        Highlighted,
        NewMessage,
    };

    enum class CopyMode {
        Everything,
        OnlyTextAndEmotes,
    };

    /// ???
    inline QString qS(const std::string& string)
    {
        return QString::fromStdString(string);
    }

    /// CONSTANTS
    const Qt::KeyboardModifiers showSplitOverlayModifiers =
        Qt::ControlModifier | Qt::AltModifier;
    const Qt::KeyboardModifiers showAddSplitRegions =
        Qt::ControlModifier | Qt::AltModifier;
    const Qt::KeyboardModifiers showResizeHandlesModifiers =
        Qt::ControlModifier;

    /// UTIL
    template <typename T>
    std::weak_ptr<T> weakOf(T* element)
    {
        return element->shared_from_this();
    }

    /// FORWARD DECL
    struct Message;
    using MessagePtr = std::shared_ptr<const Message>;

    /// OUTCOME

}  // namespace chatterino

/// STRING ALIASES
#define QStringAlias(name)                                     \
    namespace chatterino                                       \
    {                                                          \
        struct name                                            \
        {                                                      \
            QString string;                                    \
            bool operator==(const name& other) const           \
            {                                                  \
                return this->string == other.string;           \
            }                                                  \
            bool operator!=(const name& other) const           \
            {                                                  \
                return this->string != other.string;           \
            }                                                  \
        };                                                     \
    } /* namespace chatterino */                               \
    namespace std                                              \
    {                                                          \
        template <>                                            \
        struct hash<chatterino::name>                          \
        {                                                      \
            size_t operator()(const chatterino::name& s) const \
            {                                                  \
                return qHash(s.string);                        \
            }                                                  \
        };                                                     \
    } /* namespace std */

QStringAlias(UserName);
QStringAlias(UserId);
QStringAlias(Url);
QStringAlias(Tooltip);
QStringAlias(EmoteId);
QStringAlias(EmoteName);
