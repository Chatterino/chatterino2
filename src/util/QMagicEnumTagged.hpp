#pragma once

#include "util/QMagicEnum.hpp"

#include <optional>

namespace chatterino::qmagicenum {

using customize_t = std::optional<std::string_view>;

namespace detail {

namespace tag {

struct DisplayName {
};

}  // namespace tag

/// Builds the tagged data for a specific enum value (V), type (E), and Tag.
///
/// If the Tag is DisplayName, attempts to obtain a custom display name for the enum value V by calling
/// qmagicenumDisplayName(V). If no customization is provided, falls back to magic_enum's name for the value.
///
/// @tparam E Enum type
/// @tparam Tag Tag type (e.g. DisplayName)
/// @tparam V Enum value
/// @returns A compile-time static string containing the tagged data for the enum value
template <typename E, typename Tag, E V>
constexpr auto buildEnumValueTaggedData() noexcept
{
    [[maybe_unused]] constexpr auto custom = [] {
        static_assert(
            std::is_same_v<Tag, tag::DisplayName>,
            "unhandled tag in QMagicEnumTagged.hpp::enumTaggedDataValue");

        if constexpr (std::is_same_v<Tag, tag::DisplayName>)
        {
            return qmagicenumDisplayName(V);
        }
    }();

    if constexpr (custom.has_value())
    {
        constexpr auto name = *custom;
        static_assert(!name.empty(),
                      "qmagicenum::customize must return a non-empty string.");
        return magic_enum::detail::static_str<name.size()>{name};
    }
    else
    {
        // No specialization for this enum value; fall back to magic_enum's value name
        return magic_enum::detail::enum_name_v<E, V>;
    }
}

/// Stores a static std::string_view for each (enum value, Tag) pair.
///
/// For example, for MyEnum { Foo, Bar } and DisplayName tag, this will generate:
///   TAGGED_DATA_STORAGE_MyEnum_DisplayName_Foo = "Custom display name for Foo"
///   TAGGED_DATA_STORAGE_MyEnum_DisplayName_Bar = "Custom display name for Bar"
template <typename E, typename Tag, E V>
inline constexpr auto TAGGED_DATA_STORAGE =
    buildEnumValueTaggedData<E, Tag, V>();

/// Converts the static string representation of an enum value + tag into a std::array<char16_t>
template <typename C, typename E, typename Tag, E V>
consteval auto enumTaggedDataStorage()
{
    constexpr std::string_view utf8 = TAGGED_DATA_STORAGE<decltype(V), Tag, V>;

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

/// Stores a std::array<char16_t> for each (enum value, Tag) pair.
///
/// For example, for MyEnum { Foo, Bar } and DisplayName tag, this will generate:
///   TAGGED_DATA_MyEnum_DisplayName_Foo = "Custom display name for Foo" (char16_t array)
///   TAGGED_DATA_MyEnum_DisplayName_Bar = "Custom display name for Bar" (char16_t array)
template <typename E, typename Tag, E V>
inline constexpr auto TAGGED_DATA =
    enumTaggedDataStorage<char16_t, E, Tag, V>();

/// Builds a std::array<QStringView> for an enum type and tag, holding the tagged data for each enum value.
///
/// For example, for MyEnum { Foo, Bar } and DisplayName tag, this will generate:
///   INDEXED_DATA_STORAGE_MyEnum_DisplayName[0] = "Custom display name for Foo"
///   INDEXED_DATA_STORAGE_MyEnum_DisplayName[1] = "Custom display name for Bar"
template <typename E, typename Tag, std::size_t... I>
consteval auto taggedDataStorage(std::index_sequence<I...> /*unused*/)
{
    return std::array<QStringView, sizeof...(I)>{{detail::fromArray(
        TAGGED_DATA<E, Tag, magic_enum::enum_values<E>()[I]>)...}};
}

/// Stores a std::array<QStringView> for a given enum type and tag, allowing indexed access to tagged data.
///
/// For example, for MyEnum { Foo, Bar } and DisplayName tag, this will generate:
///   INDEXED_TAGGED_DATA_MyEnum_DisplayName[0] = "Custom display name for Foo"
///   INDEXED_TAGGED_DATA_MyEnum_DisplayName[1] = "Custom display name for Bar"
template <typename E, typename Tag>
inline constexpr auto INDEXED_TAGGED_DATA = taggedDataStorage<E, Tag>(
    std::make_index_sequence<magic_enum::enum_count<E>()>{});

/// Get the tagged data for a specific enum value and tag at compile time.
///
/// @tparam V Enum value
/// @tparam Tag Tag type
/// @returns The tagged data as a QStringView for the enum value.
template <detail::IsEnum auto V, typename Tag>
[[nodiscard]] consteval QStringView enumTaggedData() noexcept
{
    return QStringView{
        detail::fromArray(detail::TAGGED_DATA<decltype(V), Tag, V>)};
}

/// Get the tagged data for a runtime enum value and tag.
///
/// @tparam Tag Tag type
/// @param value Enum value
/// @returns The tagged data as a QStringView for the enum value, or an empty view if value is invalid.
template <detail::IsEnum E, typename Tag>
[[nodiscard]] constexpr QStringView enumTaggedData(E value) noexcept
{
    using D = std::decay_t<E>;

    if (const auto i = magic_enum::enum_index<D>(value))
    {
        return detail::INDEXED_TAGGED_DATA<D, Tag>[*i];
    }
    return {};
}

}  // namespace detail

/// Get the display name of a specific enum value as a QStringView at compile time.
///
/// If the enum type provides a qmagicenumDisplayName specialization, it is used. Otherwise, fall back to magic_enum's name.
///
/// @tparam V Enum value
/// @returns Display name as QStringView, or name as fallback.
template <detail::IsEnum auto V>
[[nodiscard]] consteval QStringView enumDisplayName() noexcept
{
    if constexpr (requires { qmagicenumDisplayName(V); })
    {
        return detail::enumTaggedData<V, detail::tag::DisplayName>();
    }

    // Fall back to our qmagicenum "enum name" implementation
    return enumName<V>();
}

/// Get the display name of a runtime enum value as a QStringView.
///
/// If the enum type provides a qmagicenumDisplayName specialization, it is used. Otherwise, fall back to magic_enum's name.
///
/// @param value Enum value
/// @returns Display name as QStringView, or name as fallback, or an empty string if not available.
template <detail::IsEnum E>
[[nodiscard]] constexpr QStringView enumDisplayName(E value) noexcept
{
    if constexpr (requires { qmagicenumDisplayName(value); })
    {
        using D = std::decay_t<E>;

        return detail::enumTaggedData<D, detail::tag::DisplayName>(value);
    }

    // Fall back to our qmagicenum "enum name" implementation
    return enumName(value);
}

/// Get the display name of a specific enum value as a static QString.
///
/// If the enum type provides a qmagicenumDisplayName specialization, it is used. Otherwise, fall back to magic_enum's name.
///
/// @tparam V Enum value
/// @returns Display name as QString. The returned string is static.
template <detail::IsEnum auto V>
[[nodiscard]] inline QString enumDisplayNameString() noexcept
{
    return detail::staticString(enumDisplayName<V>());
}

/// Get the display name of a runtime enum value as a static QString.
///
/// If the enum type provides a qmagicenumDisplayName specialization, it is used. Otherwise, fall back to magic_enum's name.
///
/// @param value Enum value
/// @returns Display name as QString. Returns an empty string if value is invalid. The returned string is static.
template <detail::IsEnum E>
[[nodiscard]] inline QString enumDisplayNameString(E value) noexcept
{
    using D = std::decay_t<E>;

    return detail::staticString(enumDisplayName<D>(value));
}

}  // namespace chatterino::qmagicenum
