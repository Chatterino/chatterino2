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

}  // namespace chatterino
