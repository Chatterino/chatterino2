#pragma once

#include <QColor>

#include <colorscheme.hpp>

namespace chatterino {
namespace messages {
class MessageColor
{
public:
    enum Type { Custom, Text, Link, System };

    explicit MessageColor(const QColor &color);
    explicit MessageColor(Type type = Text);

    Type getType() const;
    const QColor &getColor(ColorScheme &colorScheme) const;

private:
    Type type;
    QColor color;
};
}
}
