// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/MessageColor.hpp"

#include "messages/layouts/MessageLayoutContext.hpp"

namespace chatterino {

MessageColor::MessageColor(const QColor &color)
    : type_(Type::Custom)
    , customColor_(color)
{
}

MessageColor::MessageColor(Type type)
    : type_(type)
{
}

MessageColor MessageColor::username(const QColor &color)
{
    MessageColor c(color);
    c.type_ = Type::Username;
    return c;
}

MessageColor MessageColor::asUsernameColor() const
{
    if (this->type_ == Type::Custom)
    {
        return MessageColor::username(this->customColor_);
    }
    return *this;
}

const QColor &MessageColor::getColor(const MessageColors &colors) const
{
    switch (this->type_)
    {
        case Type::Custom:
        case Type::Username:
            return this->customColor_;
        case Type::Text:
            return colors.regularText;
        case Type::System:
            return colors.systemText;
        case Type::Link:
            return colors.linkText;
    }

    static QColor _default;
    return _default;
}

QString MessageColor::toString() const
{
    switch (this->type_)
    {
        case Type::Custom:
            return this->customColor_.name(QColor::HexArgb);
        case Type::Username:
            return QStringLiteral("username:") +
                   this->customColor_.name(QColor::HexArgb);
        case Type::Text:
            return QStringLiteral("Text");
        case Type::System:
            return QStringLiteral("System");
        case Type::Link:
            return QStringLiteral("Link");
        default:
            return {};
    }
}

QString MessageColor::toLua() const
{
    switch (this->type_)
    {
        case Type::Custom:
            return this->customColor_.name(QColor::HexArgb);
        case Type::Username:
            return QStringLiteral("username:") +
                   this->customColor_.name(QColor::HexArgb);
        case Type::Text:
            return QStringLiteral("text");
        case Type::System:
            return QStringLiteral("system");
        case Type::Link:
            return QStringLiteral("link");
        default:
            return {};
    }
}

MessageColor MessageColor::fromLua(const QString &spec, Type fallback)
{
    if (spec.isEmpty())
    {
        return fallback;
    }
    if (spec == u"text")
    {
        return MessageColor::Text;
    }
    if (spec == u"link")
    {
        return MessageColor::Link;
    }
    if (spec == u"system")
    {
        return MessageColor::System;
    }
    if (spec.startsWith(u"username:"))
    {
        return MessageColor::username(QColor(spec.mid(9)));
    }
    // custom
    return QColor(spec);
}

}  // namespace chatterino
