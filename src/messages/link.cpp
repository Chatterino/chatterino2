#include "messages/link.hpp"

namespace chatterino {
namespace messages {

Link::Link()
    : type(None)
    , value(QString())
{
}

Link::Link(Type type, const QString &value)
    : type(type)
    , value(value)
{
}

}  // namespace messages
}  // namespace chatterino
