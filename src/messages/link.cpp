#include "messages/link.hpp"

namespace chatterino {
namespace messages {

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

Link::Type Link::getType() const
{
    return this->type;
}

const QString &Link::getValue() const
{
    return this->value;
}

}  // namespace messages
}  // namespace chatterino
