#pragma once

#include <QColor>

#include "singletons/thememanager.hpp"

namespace chatterino {
namespace messages {

class MessageColor
{
public:
    enum Type { Custom, Text, Link, System };

    explicit MessageColor(const QColor &color);
    explicit MessageColor(Type type = Text);

    Type getType() const;
    const QColor &getColor(singletons::ThemeManager &themeManager) const;

private:
    Type type;
    QColor color;
};

}  // namespace messages
}  // namespace chatterino
