#include "messagecolor.hpp"

namespace chatterino {
namespace messages {
MessageColor::MessageColor(const QColor &_color)
    : type(Type::Custom)
    , color(_color)
{
}

MessageColor::MessageColor(Type _type)
    : type(_type)
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
