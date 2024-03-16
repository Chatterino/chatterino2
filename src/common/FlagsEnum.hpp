#pragma once

#include <initializer_list>
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
        for (auto flag : flags)
        {
            this->set(flag);
        }
    }

    bool operator==(const FlagsEnum<T> &other) const
    {
        return this->value_ == other.value_;
    }

    bool operator!=(const FlagsEnum<T> &other) const
    {
        return this->value_ != other.value_;
    }

    void set(T flag)
    {
        reinterpret_cast<Q &>(this->value_) |= static_cast<Q>(flag);
    }

    /** Adds the flags from `flags` in this enum. */
    void set(FlagsEnum flags)
    {
        reinterpret_cast<Q &>(this->value_) |= static_cast<Q>(flags.value_);
    }

    void unset(T flag)
    {
        reinterpret_cast<Q &>(this->value_) &= ~static_cast<Q>(flag);
    }

    void set(T flag, bool value)
    {
        if (value)
        {
            this->set(flag);
        }
        else
        {
            this->unset(flag);
        }
    }

    bool has(T flag) const
    {
        return static_cast<Q>(this->value_) & static_cast<Q>(flag);
    }

    FlagsEnum operator|(T flag)
    {
        FlagsEnum xd;
        xd.value_ = this->value_;
        xd.set(flag, true);

        return xd;
    }

    FlagsEnum operator|(FlagsEnum rhs)
    {
        return static_cast<T>(static_cast<Q>(this->value_) |
                              static_cast<Q>(rhs.value_));
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

    T value() const
    {
        return this->value_;
    }

private:
    T value_{};
};

}  // namespace chatterino
