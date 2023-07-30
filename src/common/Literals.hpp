#pragma once

#include <QString>

/// This namespace defines the string suffixes _s, _ba, and _L1 used to create Qt types at compile-time.
/// They're easier to use comapred to their corresponding macros.
///
/// * u"foobar"_s creates a QString (like QStringLiteral). The u prefix is required.
///
/// * "foobar"_ba creates a QByteArray (like QByteArrayLiteral).
///
/// * "foobar"_L1 creates a QLatin1String(-View).
namespace chatterino::literals {

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)

// This makes sure that the backing data never causes allocation after compilation.
// It's essentially the QStringLiteral macro inlined.
//
// From desktop-app/lib_base
// https://github.com/desktop-app/lib_base/blob/f904c60987115a4b514a575b23009ff25de0fafa/base/basic_types.h#L63-L152
// And qt/qtbase (5.15)
// https://github.com/qt/qtbase/blob/29400a683f96867133b28299c0d0bd6bcf40df35/src/corelib/text/qstringliteral.h#L64-L104
namespace detail {
    // NOLINTBEGIN(modernize-avoid-c-arrays)
    // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)

    template <size_t N>
    struct LiteralResolver {
        template <size_t... I>
        constexpr LiteralResolver(const char16_t (&text)[N],
                                  std::index_sequence<I...> /*seq*/)
            : utf16Text{text[I]...}
        {
        }
        template <size_t... I>
        constexpr LiteralResolver(const char (&text)[N],
                                  std::index_sequence<I...> /*seq*/)
            : latin1Text{text[I]...}
            , latin1(true)
        {
        }
        constexpr LiteralResolver(const char16_t (&text)[N])
            : LiteralResolver(text, std::make_index_sequence<N>{})
        {
        }
        constexpr LiteralResolver(const char (&text)[N])
            : LiteralResolver(text, std::make_index_sequence<N>{})
        {
        }

        const char16_t utf16Text[N]{};
        const char latin1Text[N]{};
        size_t length = N;
        bool latin1 = false;
    };

    template <size_t N>
    struct StaticStringData {
        template <std::size_t... I>
        constexpr StaticStringData(const char16_t (&text)[N],
                                   std::index_sequence<I...> /*seq*/)
            : data Q_STATIC_STRING_DATA_HEADER_INITIALIZER(N - 1)
            , text{text[I]...}
        {
        }
        QArrayData data;
        char16_t text[N];

        QStringData *pointer()
        {
            Q_ASSERT(data.ref.isStatic());
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            return static_cast<QStringData *>(&data);
        }
    };

    template <size_t N>
    struct StaticByteArrayData {
        template <std::size_t... I>
        constexpr StaticByteArrayData(const char (&text)[N],
                                      std::index_sequence<I...> /*seq*/)
            : data Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER(N - 1)
            , text{text[I]...}
        {
        }
        QByteArrayData data;
        char text[N];

        QByteArrayData *pointer()
        {
            Q_ASSERT(data.ref.isStatic());
            return &data;
        }
    };

    // NOLINTEND(cppcoreguidelines-avoid-c-arrays)
    // NOLINTEND(modernize-avoid-c-arrays)

}  // namespace detail

template <detail::LiteralResolver R>
inline QString operator""_s() noexcept
{
    static_assert(R.length > 0);  // always has a terminating null
    static_assert(!R.latin1, "QString literals must be made up of 16bit "
                             "characters. Forgot a u\"\"?");

    static auto literal = detail::StaticStringData<R.length>(
        R.utf16Text, std::make_index_sequence<R.length>{});
    return QString{QStringDataPtr{literal.pointer()}};
};

template <detail::LiteralResolver R>
inline QByteArray operator""_ba() noexcept
{
    static_assert(R.length > 0);  // always has a terminating null
    static_assert(R.latin1, "QByteArray literals must be made up of 8bit "
                            "characters. Misplaced u\"\"?");

    static auto literal = detail::StaticByteArrayData<R.length>(
        R.latin1Text, std::make_index_sequence<R.length>{});
    return QByteArray{QByteArrayDataPtr{literal.pointer()}};
};

#elif QT_VERSION < QT_VERSION_CHECK(6, 4, 0)

// The operators were added in 6.4, but their implementation works in any 6.x version.
//
// NOLINTBEGIN(cppcoreguidelines-pro-type-const-cast)
inline QString operator""_s(const char16_t *str, size_t size) noexcept
{
    return QString(
        QStringPrivate(nullptr, const_cast<char16_t *>(str), qsizetype(size)));
}

inline QByteArray operator""_ba(const char *str, size_t size) noexcept
{
    return QByteArray(
        QByteArrayData(nullptr, const_cast<char *>(str), qsizetype(size)));
}
// NOLINTEND(cppcoreguidelines-pro-type-const-cast)

#else

inline QString operator""_s(const char16_t *str, size_t size) noexcept
{
    return Qt::Literals::StringLiterals::operator""_s(str, size);
}

inline QByteArray operator""_ba(const char *str, size_t size) noexcept
{
    return Qt::Literals::StringLiterals::operator""_ba(str, size);
}

#endif

constexpr inline QLatin1String operator""_L1(const char *str,
                                             size_t size) noexcept
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using SizeType = int;
#else
    using SizeType = qsizetype;
#endif

    return QLatin1String{str, static_cast<SizeType>(size)};
}

}  // namespace chatterino::literals
