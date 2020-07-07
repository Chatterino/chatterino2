#pragma once

#include <QColor>
#include <QString>

namespace chatterino {

static const char *ANONYMOUS_USERNAME ATTR_UNUSED = "justinfan64537";

inline QByteArray getDefaultClientID()
{
    return QByteArray("7ue61iz46fz11y3cugd0l3tawb4taal");
}

static const std::vector<QColor> TWITCH_USERNAME_COLORS = {
    {255, 0, 0},      // Red
    {0, 0, 255},      // Blue
    {0, 255, 0},      // Green
    {178, 34, 34},    // FireBrick
    {255, 127, 80},   // Coral
    {154, 205, 50},   // YellowGreen
    {255, 69, 0},     // OrangeRed
    {46, 139, 87},    // SeaGreen
    {218, 165, 32},   // GoldenRod
    {210, 105, 30},   // Chocolate
    {95, 158, 160},   // CadetBlue
    {30, 144, 255},   // DodgerBlue
    {255, 105, 180},  // HotPink
    {138, 43, 226},   // BlueViolet
    {0, 255, 127},    // SpringGreen
};

}  // namespace chatterino
