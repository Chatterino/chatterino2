#include "link.h"

Link::Link()
    : m_type(None)
    , m_value(QString())
{

}

Link::Link(Type type, const QString& value)
    : m_type(type)
    , m_value(value)
{

}
