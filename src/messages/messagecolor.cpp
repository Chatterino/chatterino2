#include "messagecolor.hpp"

namespace chatterino {
namespace messages {

MessageColor::MessageColor(const QColor &_color)
    : type(Type::Custom)
    , customColor(_color)
{
}

MessageColor::MessageColor(Type _type)
    : type(_type)
{
}

const QColor &MessageColor::getColor(singletons::ThemeManager &themeManager) const
{
    switch (this->type) {
        case Type::Custom:
            return this->customColor;
        case Type::Text:
            return themeManager.messages.textColors.regular;
        case Type::System:
            return themeManager.messages.textColors.system;
        case Type::Link:
            return themeManager.messages.textColors.link;
    }

    static QColor _default;
    return _default;
}

}  // namespace messages
}  // namespace chatterino
