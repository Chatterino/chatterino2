#pragma once

#include <type_traits>

namespace chatterino {

// = std::enable_if<std::is_enum<T>::value>::type

template <typename T, typename Q = typename std::underlying_type<T>::type>
class FlagsEnum
{
public:
    FlagsEnum()
        : value(static_cast<T>(0))
    {
    }

    FlagsEnum(T value)
        : value(value)
    {
    }

    FlagsEnum(std::initializer_list<T> flags)
    {
        for (auto flag : flags) {
            this->set(flag);
        }
    }

    bool operator==(const FlagsEnum<T> &other)
    {
        return this->value == other.value;
    }

    bool operator!=(const FlagsEnum &other)
    {
        return this->value != other.value;
    }

    void set(T flag)
    {
        reinterpret_cast<Q &>(this->value) |= static_cast<Q>(flag);
    }

    void unset(T flag)
    {
        reinterpret_cast<Q &>(this->value) &= ~static_cast<Q>(flag);
    }

    void set(T flag, bool value)
    {
        if (value)
            this->set(flag);
        else
            this->unset(flag);
    }

    bool has(T flag) const
    {
        return static_cast<Q>(this->value) & static_cast<Q>(flag);
    }

    // bool hasAny(std::initializer_list<T> flags) const
    //{
    //    for (auto flag : flags) {
    //        if (this->has(flag)) return true;
    //    }
    //    return false;
    //}

    bool hasAny(FlagsEnum flags) const
    {
        return static_cast<Q>(this->value) & static_cast<Q>(flags.value);
    }

    // bool hasAll(std::initializer_list<T> flags) const
    //{
    //    for (auto flag : flags) {
    //        if (!this->has(flag)) return false;
    //    }
    //    return true;
    //}

    bool hasAll(FlagsEnum<T> flags) const
    {
        return (static_cast<Q>(this->value) & static_cast<Q>(flags.value)) &&
               static_cast<Q>(flags->value);
    }

    bool hasNone(std::initializer_list<T> flags) const
    {
        return !this->hasAny(flags);
    }

private:
    T value;
};

}  // namespace chatterino
