#pragma once

#include <magic_enum/magic_enum.hpp>
#include <QString>
#include <QStringView>

namespace chatterino::qmagicenum::detail {

template <bool, typename R>
struct EnableIfEnum {
};

template <typename R>
struct EnableIfEnum<true, R> {
    using type = R;
};

template <typename T, typename R, typename BinaryPredicate = std::equal_to<>,
          typename D = std::decay_t<T>>
using enable_if_t = typename EnableIfEnum<
    std::is_enum_v<D> &&
        std::is_invocable_r_v<bool, BinaryPredicate, QChar, QChar>,
    R>::type;

template <std::size_t N>
consteval QStringView fromArray(const std::array<char16_t, N> &arr)
{
    return QStringView{arr.data(), static_cast<QStringView::size_type>(N - 1)};
}

// Only the latin1 subset may be used right now, since it's easily convertible
template <std::size_t N>
consteval bool isLatin1(std::string_view maybe)
{
    for (std::size_t i = 0; i < N; i++)
    {
        if (maybe[i] < 0x20 || maybe[i] > 0x7e)
        {
            return false;
        }
    }
    return true;
}

template <typename BinaryPredicate>
inline constexpr bool eq(
    QStringView a, QStringView b,
    [[maybe_unused]] BinaryPredicate &&
        p) noexcept(magic_enum::detail::is_nothrow_invocable<BinaryPredicate>())
{
    // Note: operator== isn't constexpr
    if (a.size() != b.size())
    {
        return false;
    }

    for (QStringView::size_type i = 0; i < a.size(); i++)
    {
        if (!p(a[i], b[i]))
        {
            return false;
        }
    }

    return true;
}

template <typename C, typename E, E V>
consteval auto enumNameStorage()
{
    constexpr auto utf8 = magic_enum::enum_name<V>();

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

template <typename E, E V>
inline constexpr auto ENUM_NAME_STORAGE = enumNameStorage<char16_t, E, V>();

template <typename E, magic_enum::detail::enum_subtype S, std::size_t... I>
consteval auto namesStorage(std::index_sequence<I...> /*unused*/)
{
    return std::array<QStringView, sizeof...(I)>{{detail::fromArray(
        ENUM_NAME_STORAGE<E, magic_enum::enum_values<E, S>()[I]>)...}};
}

template <typename E,
          magic_enum::detail::enum_subtype S = magic_enum::detail::subtype_v<E>>
inline constexpr auto NAMES_STORAGE = namesStorage<E, S>(
    std::make_index_sequence<magic_enum::enum_count<E, S>()>{});

template <typename E, magic_enum::detail::enum_subtype S,
          typename D = std::decay_t<E>>
using NamesStorage = decltype((NAMES_STORAGE<D, S>));

template <typename Op = std::equal_to<>>
class CaseInsensitive
{
    static constexpr QChar toLower(QChar c) noexcept
    {
        return (c >= u'A' && c <= u'Z')
                   ? QChar(c.unicode() + static_cast<char16_t>(u'a' - u'A'))
                   : c;
    }

public:
    template <typename L, typename R>
    constexpr std::enable_if_t<std::is_same_v<std::decay_t<L>, QChar> &&
                                   std::is_same_v<std::decay_t<R>, QChar>,
                               bool>
        operator()(L lhs, R rhs) const noexcept
    {
        return Op{}(toLower(lhs), toLower(rhs));
    }
};

}  // namespace chatterino::qmagicenum::detail

namespace chatterino::qmagicenum {

/// @brief Get the name of an enum value
///
/// This version is much lighter on the compile times and is not restricted to the enum_range limitation.
///
/// @tparam V The enum value
/// @returns The name as a string view
template <auto V>
[[nodiscard]] consteval detail::enable_if_t<decltype(V), QStringView>
    enumName() noexcept
{
    return QStringView{
        detail::fromArray(detail::ENUM_NAME_STORAGE<decltype(V), V>)};
}

/// @brief Get the name of an enum value
///
/// @param value The enum value
/// @returns The name as a string view. If @a value does not have name or the
///          value is out of range an empty string is returned.
template <typename E,
          magic_enum::detail::enum_subtype S = magic_enum::detail::subtype_v<E>>
[[nodiscard]] constexpr detail::enable_if_t<E, QStringView> enumName(
    E value) noexcept
{
    using D = std::decay_t<E>;

    if (const auto i = magic_enum::enum_index<D, S>(value))
    {
        return detail::NAMES_STORAGE<D, S>[*i];
    }
    return {};
}

/// @brief Gets a static QString from @a view.
///
/// @pre @a view must be a static string view (i.e. it must be valid throughout
///      the entire duration of the program).
///
/// @param view The view to turn into a static string
/// @returns Qt6: A static string (never gets freed), Qt5: regular string
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
[[nodiscard]] inline QString staticString(QStringView view) noexcept
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    return QString(QStringPrivate(nullptr, const_cast<char16_t *>(view.utf16()),
                                  view.size()));
}
#else
[[nodiscard]] inline QString staticString(QStringView view)
{
    return view.toString();
}
#endif

/// @brief Get the name of an enum value
///
/// This version is much lighter on the compile times and is not restricted to
/// the enum_range limitation.
///
/// @tparam V The enum value
/// @returns The name as a string. The returned string is static.
template <auto V>
[[nodiscard]] inline detail::enable_if_t<decltype(V), QString>
    enumNameString() noexcept
{
    return staticString(enumName<V>());
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
template <typename E,
          magic_enum::detail::enum_subtype S = magic_enum::detail::subtype_v<E>>
[[nodiscard]] inline detail::enable_if_t<E, QString> enumNameString(
    E value) noexcept
{
    using D = std::decay_t<E>;

    return staticString(enumName<D, S>(value));
}

/// @brief Gets the enum value from a name
///
/// @tparam E The enum type to parse the @a name as
/// @param name The name of the enum value to parse
/// @param p A predicate to compare characters of a string
///          (defaults to std::equal_to)
/// @returns A `std::optional` of the parsed value. If no value was parsed,
///          `std::nullopt` is returned.
template <typename E,
          magic_enum::detail::enum_subtype S = magic_enum::detail::subtype_v<E>,
          typename BinaryPredicate = std::equal_to<>>
[[nodiscard]] constexpr detail::enable_if_t<E, std::optional<std::decay_t<E>>,
                                            BinaryPredicate>
    enumCast(QStringView name,
             [[maybe_unused]] BinaryPredicate p =
                 {}) noexcept(magic_enum::detail::
                                  is_nothrow_invocable<BinaryPredicate>())
{
    using D = std::decay_t<E>;

    if constexpr (magic_enum::enum_count<D, S>() == 0)
    {
        static_cast<void>(name);
        return std::nullopt;  // Empty enum.
    }

    for (std::size_t i = 0; i < magic_enum::enum_count<D, S>(); i++)
    {
        if (detail::eq(name, detail::NAMES_STORAGE<D, S>[i], p))
        {
            return magic_enum::enum_value<D, S>(i);
        }
    }
    return std::nullopt;  // Invalid value or out of range.
}

/// @brief Constructs a name from the @a flags
///
/// @param flags The combined flags to construct the name from
/// @param sep A separator between each flag (defaults to u'|')
/// @returns A string containing all names separated by @a sep. If any flag in
///          @a flags is out of rage or does not have a name, an empty string
///          is returned.
template <typename E>
[[nodiscard]] inline detail::enable_if_t<E, QString> enumFlagsName(
    E flags, char16_t sep = u'|')
{
    using D = std::decay_t<E>;
    using U = std::underlying_type_t<D>;
    constexpr auto S = magic_enum::detail::enum_subtype::flags;  // NOLINT

    QString name;
    auto checkValue = U{0};
    for (std::size_t i = 0; i < magic_enum::enum_count<D, S>(); ++i)
    {
        const auto v = static_cast<U>(magic_enum::enum_value<D, S>(i));
        if ((static_cast<U>(flags) & v) != 0)
        {
            const auto n = detail::NAMES_STORAGE<D, S>[i];
            if (!n.empty())
            {
                checkValue |= v;
                if (!name.isEmpty())
                {
                    name.append(sep);
                }
                name.append(n);
            }
            else
            {
                return {};  // Value out of range.
            }
        }
    }

    if (checkValue != 0 && checkValue == static_cast<U>(flags))
    {
        return name;
    }
    return {};  // Invalid value or out of range.
}

/// @brief Get the names of all values from @a E.
///
/// @tparam E The enum type
/// @returns A `std::array` of all names (`QStringView`s)
template <typename E,
          magic_enum::detail::enum_subtype S = magic_enum::detail::subtype_v<E>>
[[nodiscard]] constexpr auto enumNames() noexcept
    -> detail::enable_if_t<E, detail::NamesStorage<E, S>>
{
    return detail::NAMES_STORAGE<std::decay_t<E>, S>;
}

/// Allows you to write qmagicenum::enumCast<foo>("bar", qmagicenum::CASE_INSENSITIVE)
inline constexpr auto CASE_INSENSITIVE = detail::CaseInsensitive<>{};

}  // namespace chatterino::qmagicenum
