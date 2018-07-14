#pragma once

#include "debug/Log.hpp"

#include <QString>
#include <QWidget>
#include <boost/preprocessor.hpp>

#include <string>

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

const Qt::KeyboardModifiers showSplitOverlayModifiers = Qt::ControlModifier | Qt::AltModifier;
const Qt::KeyboardModifiers showAddSplitRegions = Qt::ControlModifier | Qt::AltModifier;
const Qt::KeyboardModifiers showResizeHandlesModifiers = Qt::ControlModifier;

static const char *ANONYMOUS_USERNAME_LABEL ATTR_UNUSED = " - anonymous - ";

#define return_if(condition) \
    if ((condition)) {       \
        return;              \
    }

#define return_unless(condition) \
    if (!(condition)) {          \
        return;                  \
    }

}  // namespace chatterino
