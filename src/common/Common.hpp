#pragma once

#include <boost/preprocessor.hpp>
#include <QString>
#include <QWidget>

#include <memory>
#include <optional>
#include <string>

#define LINK_CHATTERINO_WIKI "https://wiki.chatterino.com"
#define LINK_CHATTERINO_DISCORD "https://discord.gg/7Y5AYhAK4z"
#define LINK_CHATTERINO_SOURCE "https://github.com/Chatterino/chatterino2"

namespace chatterino {

const inline auto TWITCH_PLAYER_URL =
    QStringLiteral("https://player.twitch.tv/?channel=%1&parent=twitch.tv");

enum class HighlightState {
    None,
    Highlighted,
    NewMessage,
};

const Qt::KeyboardModifiers showSplitOverlayModifiers =
    Qt::ControlModifier | Qt::AltModifier;
const Qt::KeyboardModifiers showAddSplitRegions =
    Qt::ControlModifier | Qt::AltModifier;
const Qt::KeyboardModifiers showResizeHandlesModifiers = Qt::ControlModifier;

#ifndef ATTR_UNUSED
#    ifdef Q_OS_WIN
#        define ATTR_UNUSED
#    else
#        define ATTR_UNUSED __attribute__((unused))
#    endif
#endif

static const char *ANONYMOUS_USERNAME_LABEL ATTR_UNUSED = " - anonymous - ";

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
