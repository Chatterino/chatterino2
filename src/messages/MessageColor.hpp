#pragma once

#include "singletons/Theme.hpp"

#include <QColor>

namespace chatterino {

struct MessageColor {
    enum Type { Custom, Text, Link, System };

    MessageColor(const QColor &color);
    MessageColor(Type type = Text);

    const QColor &getColor(Theme &themeManager) const;

private:
    Type type;
    QColor customColor;
};

}  // namespace chatterino
