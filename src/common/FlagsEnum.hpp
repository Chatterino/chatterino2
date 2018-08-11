#pragma once

#include <type_traits>

namespace chatterino {

template <typename T, typename Q = typename std::underlying_type<T>::type>
class FlagsEnum
{
public:
    FlagsEnum()
        : value_(static_cast<T>(0))
    {
    }

    FlagsEnum(T value)
        : value_(value)
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
        return this->value_ == other.value_;
    }

    bool operator!=(const FlagsEnum &other)
    {
        return this->value_ != other.value_;
    }

    void set(T flag)
    {
        reinterpret_cast<Q &>(this->value_) |= static_cast<Q>(flag);
    }

    void unset(T flag)
    {
        reinterpret_cast<Q &>(this->value_) &= ~static_cast<Q>(flag);
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
        return static_cast<Q>(this->value_) & static_cast<Q>(flag);
    }

    bool hasAny(FlagsEnum flags) const
    {
        return static_cast<Q>(this->value_) & static_cast<Q>(flags.value_);
    }

    bool hasAll(FlagsEnum<T> flags) const
    {
        return (static_cast<Q>(this->value_) & static_cast<Q>(flags.value_)) &&
               static_cast<Q>(flags->value);
    }

    bool hasNone(std::initializer_list<T> flags) const
    {
        return !this->hasAny(flags);
    }

private:
    T value_{};
};

}  // namespace chatterino
