#pragma once
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "util/FunctionRef.hpp"
#    include "util/QMagicEnum.hpp"
#    include "util/TypeName.hpp"

#    include <nonstd/expected.hpp>
#    include <QObject>
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

namespace chatterino {

class Plugin;
struct Link;

}  // namespace chatterino

namespace chatterino::lua {

class ThisPluginState
{
public:
    ThisPluginState(lua_State *Ls)
        : plugptr_(nullptr)
        , state_(Ls)
    {
    }

    operator lua_State *() const noexcept
    {
        return this->state_;
    }

    lua_State *operator->() const noexcept
    {
        return this->state_;
    }
    lua_State *state() const noexcept
    {
        return this->state_;
    }

    Plugin *plugin();

private:
    Plugin *plugptr_;
    lua_State *state_;
};

QString errorResultToString(const sol::protected_function_result &result);

/// @brief Attempts to call @a function with @a args
///
/// @a T is expected to be returned.
/// If `void` is specified, the returned values
/// are ignored.
/// `std::optional<T>` means nil|LuaEquiv<T> (or zero returns)
/// A return type that doesn't match returns an error
template <typename T, typename... Args>
inline nonstd::expected_lite::expected<T, QString> tryCall(const auto &function,
                                                           Args &&...args)
    requires(std::same_as<std::remove_cvref_t<decltype(function)>,
                          sol::protected_function> ||
             std::same_as<std::remove_cvref_t<decltype(function)>,
                          sol::main_protected_function>)
{
    sol::protected_function_result result =
        function(std::forward<Args>(args)...);
    if (!result.valid())
    {
        return nonstd::expected_lite::make_unexpected(
            errorResultToString(result));
    }

    if constexpr (std::is_same_v<T, void>)
    {
        return {};
    }
    else
    {
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
                QString::number(result.return_count()) %
                u" values were returned");
        }

        try
        {
            if constexpr (detail::IsOptional<T>)
            {
                // we want to error on anything that is not nil|T,
                // std::optional<T> in sol means "give me a T or if it does not match nullopt"
                if (result.get_type() == sol::type::nil)
                {
                    return {};
                }
                auto ret = result.get<T>();

                if (!ret)
                {
                    auto t = type_name<T>();
                    return nonstd::expected_lite::make_unexpected(
                        u"Expected " % QLatin1String(t.data(), t.size()) %
                        u" to be returned but " %
                        qmagicenum::enumName(result.get_type()) %
                        u" was returned");
                }
                return *ret;
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
                        qmagicenum::enumName(result.get_type()) %
                        u" was returned");
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
}

void logError(Plugin *plugin, QStringView context, const QString &msg);

template <typename T>
bool hasValueOrLog(const nonstd::expected<T, QString> &res, QStringView context,
                   Plugin *plugin)
{
    if (!res.has_value())
    {
        logError(plugin, context, res.error());
        return false;
    }
    return true;
}

void loggedVoidCall(const auto &fn, QStringView context, Plugin *plugin,
                    auto &&...args)
{
    auto res = tryCall<void>(fn, std::forward<decltype(args)>(args)...);
    hasValueOrLog(res, context, plugin);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#    define SOL_STACK_FUNCTIONS(TYPE)                                 \
        bool sol_lua_check(                                           \
            sol::types<TYPE>, lua_State *L, int index,                \
            chatterino::FunctionRef<sol::check_handler_type> handler, \
            sol::stack::record &tracking);                            \
        TYPE sol_lua_get(sol::types<TYPE>, lua_State *L, int index,   \
                         sol::stack::record &tracking);               \
        int sol_lua_push(sol::types<TYPE>, lua_State *L, const TYPE &value);

SOL_STACK_FUNCTIONS(chatterino::lua::ThisPluginState)

}  // namespace chatterino::lua

namespace chatterino {

SOL_STACK_FUNCTIONS(chatterino::Link)

}  // namespace chatterino

SOL_STACK_FUNCTIONS(QString)
SOL_STACK_FUNCTIONS(QStringList)
SOL_STACK_FUNCTIONS(QByteArray)

#    undef SOL_STACK_FUNCTIONS

#endif
