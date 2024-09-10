#pragma once

#include <boost/preprocessor.hpp>
#include <QString>
#include <QWidget>

#include <memory>

namespace chatterino {

inline constexpr QStringView LINK_CHATTERINO_WIKI =
    u"https://wiki.chatterino.com";
inline constexpr QStringView LINK_CHATTERINO_DISCORD =
    u"https://discord.gg/7Y5AYhAK4z";
inline constexpr QStringView LINK_CHATTERINO_SOURCE =
    u"https://github.com/Chatterino/chatterino2";

inline constexpr QStringView TWITCH_PLAYER_URL =
    u"https://player.twitch.tv/?channel=%1&parent=twitch.tv";

enum class HighlightState {
    None,
    Highlighted,
    NewMessage,
};

inline constexpr Qt::KeyboardModifiers SHOW_SPLIT_OVERLAY_MODIFIERS =
    Qt::ControlModifier | Qt::AltModifier;
inline constexpr Qt::KeyboardModifiers SHOW_ADD_SPLIT_REGIONS =
    Qt::ControlModifier | Qt::AltModifier;
inline constexpr Qt::KeyboardModifiers SHOW_RESIZE_HANDLES_MODIFIERS =
    Qt::ControlModifier;

inline constexpr const char *ANONYMOUS_USERNAME_LABEL = " - anonymous - ";

template <typename T>
std::weak_ptr<T> weakOf(T *element)
{
    return element->shared_from_this();
}

struct Message;
using MessagePtr = std::shared_ptr<const Message>;

enum class CopyMode {
    Everything,
    EverythingButReplies,
    OnlyTextAndEmotes,
};

struct DeleteLater {
    void operator()(QObject *obj)
    {
        obj->deleteLater();
    }
};

template <typename T>
using QObjectPtr = std::unique_ptr<T, DeleteLater>;

}  // namespace chatterino
