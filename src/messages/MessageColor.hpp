#pragma once

#include "singletons/ThemeManager.hpp"

#include <QColor>

namespace chatterino {

struct MessageColor {
    enum Type { Custom, Text, Link, System };

    MessageColor(const QColor &color);
    MessageColor(Type type = Text);

    const QColor &getColor(chatterino::ThemeManager &themeManager) const;

private:
    Type type;
    QColor customColor;
};

}  // namespace chatterino
