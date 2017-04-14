#include "messages/link.h"

namespace  chatterino {
namespace  messages {

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
}
}
