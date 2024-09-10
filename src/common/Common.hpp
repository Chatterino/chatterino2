#pragma once

#include <boost/preprocessor.hpp>
#include <QString>
#include <QWidget>

#include <memory>

namespace chatterino {

inline constexpr auto &LINK_CHATTERINO_WIKI_DATA =
    u"https://wiki.chatterino.com";
inline constexpr auto &LINK_CHATTERINO_DISCORD_DATA =
    u"https://discord.gg/7Y5AYhAK4z";
inline constexpr auto &LINK_CHATTERINO_SOURCE_DATA =
    u"https://github.com/Chatterino/chatterino2";

inline constexpr auto &TWITCH_PLAYER_URL_DATA =
    u"https://player.twitch.tv/?channel=%1&parent=twitch.tv";

inline constexpr QStringView LINK_CHATTERINO_WIKI = LINK_CHATTERINO_WIKI_DATA;
inline constexpr QStringView LINK_CHATTERINO_DISCORD =
    LINK_CHATTERINO_DISCORD_DATA;
inline constexpr QStringView LINK_CHATTERINO_SOURCE =
    LINK_CHATTERINO_SOURCE_DATA;

inline constexpr QStringView TWITCH_PLAYER_URL = TWITCH_PLAYER_URL_DATA;

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
