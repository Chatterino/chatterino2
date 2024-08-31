#pragma once

#include <concepts>
#include <type_traits>

namespace chatterino {

template <typename T>
    requires std::is_enum_v<T>
class FlagsEnum
{
public:
    using Int = std::underlying_type_t<T>;

    constexpr FlagsEnum() noexcept = default;

    constexpr FlagsEnum(std::convertible_to<T> auto... flags) noexcept
        : value_(
              static_cast<T>((static_cast<Int>(static_cast<T>(flags)) | ...)))
    {
    }

    friend constexpr bool operator==(FlagsEnum lhs, FlagsEnum rhs) noexcept
    {
        return lhs.value_ == rhs.value_;
    }
    friend constexpr bool operator!=(FlagsEnum lhs, FlagsEnum rhs) noexcept
    {
        return lhs.value_ != rhs.value_;
    }

    friend constexpr bool operator==(FlagsEnum lhs, T rhs) noexcept
    {
        return lhs.value_ == rhs;
    }
    friend constexpr bool operator!=(FlagsEnum lhs, T rhs) noexcept
    {
        return lhs.value_ != rhs;
    }

    friend constexpr bool operator==(T lhs, FlagsEnum rhs) noexcept
    {
        return lhs == rhs.value_;
    }
    friend constexpr bool operator!=(T lhs, FlagsEnum rhs) noexcept
    {
        return lhs != rhs.value_;
    }

    constexpr void set(std::convertible_to<T> auto... flags) noexcept
    {
        this->value_ =
            static_cast<T>(static_cast<Int>(this->value_) |
                           (static_cast<Int>(static_cast<T>(flags)) | ...));
    }

    /** Adds the flags from `flags` in this enum. */
    constexpr void set(FlagsEnum flags) noexcept
    {
        this->value_ = static_cast<T>(static_cast<Int>(this->value_) |
                                      static_cast<Int>(flags.value_));
    }

    constexpr void unset(std::convertible_to<T> auto... flags) noexcept
    {
        this->value_ =
            static_cast<T>(static_cast<Int>(this->value_) &
                           ~(static_cast<Int>(static_cast<T>(flags)) | ...));
    }

    constexpr void set(T flag, bool value) noexcept
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

    constexpr FlagsEnum operator|(T flag) const noexcept
    {
        return static_cast<T>(static_cast<Int>(this->value_) |
                              static_cast<Int>(flag));
    }

    constexpr FlagsEnum operator|(FlagsEnum rhs) const noexcept
    {
        return static_cast<T>(static_cast<Int>(this->value_) |
                              static_cast<Int>(rhs.value_));
    }

    constexpr bool has(T flag) const noexcept
    {
        return static_cast<Int>(this->value_) & static_cast<Int>(flag);
    }

    constexpr bool hasAny(FlagsEnum flags) const noexcept
    {
        return (static_cast<Int>(this->value_) &
                static_cast<Int>(flags.value_)) != 0;
    }

    constexpr bool hasAny(std::convertible_to<T> auto... flags) const noexcept
    {
        return this->hasAny(FlagsEnum{flags...});
    }

    constexpr bool hasAll(FlagsEnum flags) const noexcept
    {
        return (static_cast<Int>(this->value_) &
                static_cast<Int>(flags.value_)) ==
               static_cast<Int>(flags.value_);
    }

    constexpr bool hasAll(std::convertible_to<T> auto... flags) const noexcept
    {
        return this->hasAll(FlagsEnum{flags...});
    }

    constexpr bool hasNone(FlagsEnum flags) const noexcept
    {
        return (static_cast<Int>(this->value_) &
                static_cast<Int>(flags.value_)) == 0;
    }

    constexpr bool hasNone() const noexcept = delete;
    constexpr bool hasNone(std::convertible_to<T> auto... flags) const noexcept
    {
        return this->hasNone(FlagsEnum{flags...});
    }

    /// Returns true if the enum has no flag set (i.e. its underlying value is 0)
    constexpr bool isEmpty() const noexcept
    {
        return static_cast<Int>(this->value_) == 0;
    }

    constexpr T value() const noexcept
    {
        return this->value_;
    }

private:
    T value_{};
};

}  // namespace chatterino
