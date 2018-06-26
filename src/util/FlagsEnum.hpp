#pragma once

#include <type_traits>

namespace chatterino {
namespace util {

// = std::enable_if<std::is_enum<T>::value>::type

template <typename T, typename Q = typename std::underlying_type<T>::type>
class FlagsEnum
{
public:
    FlagsEnum()
        : value(static_cast<T>(0))
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

    void EnableFlag(T flag)
    {
        reinterpret_cast<Q &>(this->value) |= static_cast<Q>(flag);
    }

    bool HasFlag(Q flag) const
    {
        return (this->value & flag) == flag;
    }

    T value;
};

}  // namespace util
}  // namespace chatterino
