#pragma once

#include "boost/noncopyable.hpp"

namespace chatterino {
namespace util {

template <typename T>
class Property final : boost::noncopyable
{
public:
    Property()
    {
    }

    Property(const T &_value)
        : value(_value)
    {
    }

    T &operator=(const T &f)
    {
        return value = f;
    }

    operator T const &() const
    {
        return value;
    }

protected:
    T value;
};

}  // namespace util
}  // namespace chatterino
