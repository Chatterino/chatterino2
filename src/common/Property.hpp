#pragma once

#include "boost/noncopyable.hpp"

namespace chatterino {

template <typename T>
class Property final : boost::noncopyable
{
public:
    Property()
    {
    }

    Property(const T &value)
        : value_(value)
    {
    }

    T &operator=(const T &f)
    {
        return value_ = f;
    }

    operator T const &() const
    {
        return value_;
    }

protected:
    T value_;
};

}  // namespace chatterino
