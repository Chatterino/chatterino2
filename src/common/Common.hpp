#pragma once

#include <QString>
#include <QWidget>
#include <boost/optional.hpp>
#include <boost/preprocessor.hpp>
#include <string>

#include "common/Aliases.hpp"
#include "common/Outcome.hpp"
#include "common/ProviderId.hpp"

namespace chatterino {

enum class HighlightState {
    None,
    Highlighted,
    NewMessage,
};

inline QString qS(const std::string &string)
{
    return QString::fromStdString(string);
}

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
