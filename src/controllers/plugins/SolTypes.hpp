#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS

#    include "util/QMagicEnum.hpp"
#    include "util/TypeName.hpp"

#    include <nonstd/expected.hpp>
#    include <QString>
#    include <QStringBuilder>
#    include <QStringList>
#    include <sol/sol.hpp>

namespace chatterino::detail {

// NOLINTBEGIN(readability-identifier-naming)
template <typename T>
constexpr bool IsOptional = false;
template <typename T>
constexpr bool IsOptional<std::optional<T>> = true;
// NOLINTEND(readability-identifier-naming)

}  // namespace chatterino::detail

namespace chatterino::lua {

/// @brief Attempts to call @a function with @a args
///
/// @a T is expected to be returned.
/// If `void` is specified, the returned values
/// are ignored.
/// `std::optional` signifies zero or one returned values.
template <typename T, typename... Args>
inline nonstd::expected_lite::expected<T, QString> tryCall(
    const sol::protected_function &function, Args &&...args)
{
    sol::protected_function_result result =
        function(std::forward<Args>(args)...);
    if (!result.valid())
    {
        sol::error err = result;
        return nonstd::expected_lite::make_unexpected(
            QString::fromUtf8(err.what()));
    }

    if constexpr (std::is_same_v<T, void>)
    {
        return {};
    }
    if constexpr (detail::IsOptional<T>)
    {
        if (result.return_count() == 0)
        {
            return {};
        }
    }

    if (result.return_count() > 1)
    {
        return nonstd::expected_lite::make_unexpected(
            u"Expected one value to be returned but " %
            QString::number(result.return_count()) % u" values were returned");
    }

    try
    {
        // XXX: this has weird failure modes,
        // std::optional<T> means this is fallible, but we want nil|LuaFor<T>
        if constexpr (detail::IsOptional<T>)
        {
            return result.get<T>();
        }
        else
        {
            auto ret = result.get<std::optional<T>>();

            if (!ret)
            {
                auto t = type_name<T>();
                return nonstd::expected_lite::make_unexpected(
                    u"Expected " % QLatin1String(t.data(), t.size()) %
                    u" to be returned but " %
                    qmagicenum::enumName(result.get_type()) % u" was returned");
            }
            return *ret;
        }
    }
    catch (std::runtime_error &e)
    {
        return nonstd::expected_lite::make_unexpected(
            QString::fromUtf8(e.what()));
    }
    // non other exceptions we let it explode
}

}  // namespace chatterino::lua

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#    define SOL_STACK_FUNCTIONS(TYPE)                                      \
        bool sol_lua_check(sol::types<TYPE>, lua_State *L, int index,      \
                           std::function<sol::check_handler_type> handler, \
                           sol::stack::record &tracking);                  \
        TYPE sol_lua_get(sol::types<TYPE>, lua_State *L, int index,        \
                         sol::stack::record &tracking);                    \
        int sol_lua_push(sol::types<TYPE>, lua_State *L, const TYPE &value);

SOL_STACK_FUNCTIONS(QString)
SOL_STACK_FUNCTIONS(QStringList)

#    undef SOL_STACK_FUNCTIONS

#endif
