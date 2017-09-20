#include "messagecolor.h"

namespace chatterino {
namespace messages {
MessageColor::MessageColor(const QColor &color)
    : type(Type::Custom)
    , color(color)
{
}

MessageColor::MessageColor(Type type)
    : type(type)
{
}

MessageColor::Type MessageColor::getType() const
{
    return this->type;
}

const QColor &MessageColor::getColor(ColorScheme &colorScheme) const
{
    switch (this->type) {
        case Type::Custom:
            return this->color;
        case Type::Text:
            return colorScheme.Text;
        case Type::System:
            return colorScheme.SystemMessageColor;
        case Type::Link:
            return colorScheme.TextLink;
    }

    static QColor _default;
    return _default;
}
}
}
