#pragma once

#include <QBrush>

#include <type_traits>

namespace chatterino {

/// The values must always be castable to a Qt::BrushStyle
// NOLINTNEXTLINE(performance-enum-size)
enum class LastMessageLineStyle : std::underlying_type_t<Qt::BrushStyle> {
    Solid = Qt::SolidPattern,
    Dotted = Qt::VerPattern,
};

}  // namespace chatterino
