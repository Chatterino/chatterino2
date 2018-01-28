#pragma once

#include <type_traits>

namespace chatterino {
namespace util {

template <typename T, typename _ = std::enable_if<std::is_enum<T>::value>::type,
          typename Q = std::underlying_type<T>::type>
class FlagsEnum
{
public:
    FlagsEnum()
        : value((T)0)
    {
    }

    FlagsEnum(T _value)
        : value(_value)
    {
    }

    inline T operator~() const
    {
        return (T) ~(Q)this->value;
    }
    inline T operator|(Q a) const
    {
        return (T)((Q)a | (Q)this->value);
    }
    inline T operator&(Q a) const
    {
        return (T)((Q)a & (Q)this->value);
    }
    inline T operator^(Q a) const
    {
        return (T)((Q)a ^ (Q)this->value);
    }
    inline T &operator|=(const Q &a)
    {
        return (T &)((Q &)this->value |= (Q)a);
    }
    inline T &operator&=(const Q &a)
    {
        return (T &)((Q &)this->value &= (Q)a);
    }
    inline T &operator^=(const Q &a)
    {
        return (T &)((Q &)this->value ^= (Q)a);
    }

    T value;
};
}  // namespace util
}  // namespace chatterino
