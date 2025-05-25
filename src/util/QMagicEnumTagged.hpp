#pragma once

#include "util/QMagicEnum.hpp"

namespace chatterino::qmagicenum {

namespace tag {

struct DisplayName {
};

}  // namespace tag

namespace customize {

template <typename E, typename Tag>
constexpr magic_enum::customize::customize_t enumTaggedData(E /*v*/) noexcept
{
    return magic_enum::customize::default_tag;
}

}  // namespace customize

namespace detail {

template <typename E, typename Tag, E V>
constexpr auto enumTaggedDataValue() noexcept
{
    [[maybe_unused]] constexpr auto custom =
        customize::enumTaggedData<E, Tag>(V);

    static_assert(std::is_same_v<std::decay_t<decltype(custom)>,
                                 magic_enum::customize::customize_t>,
                  "qmagicenum::customize requires magic_enum::customize_t "
                  "return value type.");

    if constexpr (custom.first ==
                  magic_enum::customize::detail::customize_tag::custom_tag)
    {
        constexpr auto name = custom.second;
        static_assert(!name.empty(),
                      "qmagicenum::customize must return a non-empty string.");
        return magic_enum::detail::static_str<name.size()>{name};
    }
    else if constexpr (custom.first == magic_enum::customize::detail::
                                           customize_tag::invalid_tag)
    {
        return magic_enum::detail::static_str<0>{};
    }
    else if constexpr (custom.first == magic_enum::customize::detail::
                                           customize_tag::default_tag)
    {
        // Fallback to magic_enum::enum_name
        return magic_enum::detail::enum_name_v<E, V>;
    }
    else
    {
        static_assert(magic_enum::detail::always_false_v<E>,
                      "qmagicenum::customize implementation invalid.");
    }
}

template <typename E, typename Tag, E V>
inline constexpr auto TAGGED_DATA_V = enumTaggedDataValue<E, Tag, V>();

template <typename C, typename E, typename Tag, E V>
consteval auto enumTaggedDataStorage()
{
    constexpr std::string_view utf8 = TAGGED_DATA_V<decltype(V), Tag, V>;

    static_assert(isLatin1<utf8.size()>(utf8),
                  "Can't convert non-latin1 UTF8 to UTF16");

    std::array<C, utf8.size() + 1> storage;
    for (std::size_t i = 0; i < utf8.size(); i++)
    {
        storage[i] = static_cast<C>(utf8[i]);
    }
    storage[utf8.size()] = 0;
    return storage;
}

/// This contains a std::array<char16_t> for each enum value + Tag
template <typename E, typename Tag, E V>
inline constexpr auto ENUM_TAGGED_DATA_STORAGE =
    enumTaggedDataStorage<char16_t, E, Tag, V>();

template <typename E, typename Tag, std::size_t... I>
consteval auto taggedDataStorage(std::index_sequence<I...> /*unused*/)
{
    return std::array<QStringView, sizeof...(I)>{{detail::fromArray(
        ENUM_TAGGED_DATA_STORAGE<E, Tag, magic_enum::enum_values<E>()[I]>)...}};
}

/// This contains a std::array<QStringView> for each enum + Tag
template <typename E, typename Tag>
inline constexpr auto TAGGED_DATA_STORAGE = taggedDataStorage<E, Tag>(
    std::make_index_sequence<magic_enum::enum_count<E>()>{});

/// @brief Get the name of an enum value TODO
///
/// This version is much lighter on the compile times and is not restricted to the enum_range limitation.
///
/// @tparam V The enum value
/// @returns The name as a string view
template <detail::IsEnum auto V, typename Tag>
[[nodiscard]] consteval QStringView enumTaggedData() noexcept
{
    return QStringView{detail::fromArray(
        detail::ENUM_TAGGED_DATA_STORAGE<decltype(V), Tag, V>)};
}

/// @brief Get the tagged data of an enum value
///
/// @tparam Tag The tag (i.e. type) of data to return
/// @param value The enum value
/// @returns The tagged data as a string view.
///          If @a value does not have a display name, its name will be returned.
///          If @a value does not have a name, or if it's out of range, an empty
///          string is returned.
template <detail::IsEnum E, typename Tag>
[[nodiscard]] constexpr QStringView enumTaggedData(E value) noexcept
{
    using D = std::decay_t<E>;

    if (const auto i = magic_enum::enum_index<D>(value))
    {
        return detail::TAGGED_DATA_STORAGE<D, Tag>[*i];
    }
    return {};
}

}  // namespace detail

/// @brief Get the display name of an enum value
///
/// This version is much lighter on the compile times and is not restricted to the enum_range limitation.
///
/// @tparam V The enum value
/// @returns The display name as a string view.
///          If @a value does not have a display name, its name will be returned.
///          If @a value does not have a name, or if it's out of range, an empty
///          string is returned.
template <detail::IsEnum auto V>
[[nodiscard]] consteval QStringView enumDisplayName() noexcept
{
    return detail::enumTaggedData<V, tag::DisplayName>();
}

/// @brief Get the display name of an enum value
///
/// @param value The enum value
/// @returns The display name as a string view.
///          If @a value does not have a display name, its name will be returned.
///          If @a value does not have a name, or if it's out of range, an empty
///          string is returned.
template <detail::IsEnum E>
[[nodiscard]] constexpr QStringView enumDisplayName(E value) noexcept
{
    using D = std::decay_t<E>;

    return detail::enumTaggedData<D, tag::DisplayName>(value);
}

/// @brief Get the name of an enum value
///
/// This version is much lighter on the compile times and is not restricted to
/// the enum_range limitation.
///
/// @tparam V The enum value
/// @returns The name as a string. The returned string is static.
template <detail::IsEnum auto V>
[[nodiscard]] inline QString enumDisplayNameString() noexcept
{
    return detail::staticString(enumDisplayName<V>());
}

/// @brief Get the name of an enum value
///
/// This version is much lighter on the compile times and is not restricted to
/// the enum_range limitation.
///
/// @tparam V The enum value
/// @returns The name as a string. If @a value does not have name or the
///          value is out of range an empty string is returned.
///          The returned string is static.
template <detail::IsEnum E>
[[nodiscard]] inline QString enumDisplayNameString(E value) noexcept
{
    using D = std::decay_t<E>;

    return detail::staticString(enumDisplayName<D>(value));
}

}  // namespace chatterino::qmagicenum
