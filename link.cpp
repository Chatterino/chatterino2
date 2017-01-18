#include "link.h"

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
