#pragma once

#include <QColor>

#include "singletons/thememanager.hpp"

namespace chatterino {
namespace messages {
struct MessageColor {
    enum Type { Custom, Text, Link, System };

    MessageColor(const QColor &color);
    MessageColor(Type type = Text);

    const QColor &getColor(singletons::ThemeManager &themeManager) const;

private:
    Type type;
    QColor customColor;
};
}  // namespace messages
}  // namespace chatterino
