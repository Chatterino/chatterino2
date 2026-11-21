// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "messages/Link.hpp"

namespace chatterino {

Link::Link()
    : type(None)
    , value(QString())
{
}

Link::Link(Type _type, const QString &_value)
    : type(_type)
    , value(_value)
{
}

bool Link::isValid() const
{
    return this->type != None;
}

bool Link::isUrl() const
{
    return this->type == Url;
}

}  // namespace chatterino
