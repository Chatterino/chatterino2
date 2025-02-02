#pragma once

#include <boost/json.hpp>
#include <QString>

#include <cassert>
#include <string>
#include <variant>

namespace chatterino::eventsub::lib {

// Adapted from: https://stackoverflow.com/a/56766138.
// NOTE: Relies on the "magic" prefixes and suffixes. There are implementations
// that attempt to manually detect these (linked in the SO answer above) but
// they seemed too complex for the scope we have here.
template <typename T>
constexpr auto type_name()
{
    std::string_view name, prefix, suffix;
#ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto chatterino::type_name() [T = ";
    suffix = "]";
#elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto chatterino::type_name() [with T = ";
    suffix = "]";
#elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl chatterino::type_name<";
    suffix = ">(void)";
#endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());

    if (name.starts_with("class "))
    {
        name.remove_prefix(6);
    }
    if (name.starts_with("struct "))
    {
        name.remove_prefix(7);
    }

    return name;
}

/// String is a struct that holds either an std::string or a QString
///
/// The intended use is for it to receive an std::string that has been built by
/// boost::json, and once it's been passed into a GUI appliciaton, it can use
/// the `qt` function to convert the backing string to a QString,
/// while we ensure the conversion is only done once.
struct String {
    explicit String(std::string &&v)
        : backingString(std::move(v))
    {
    }

    ~String() = default;

    String(const String &s) = delete;
    String(String &&s) = default;

    String &operator=(const String &) = delete;
    String &operator=(String &&) = default;

    /// Returns the string as a QString, modifying the backing string to ensure
    /// the copy only happens once.
    QString qt() const
    {
        return std::visit(
            [this](auto &&arg) -> QString {
                using T = std::decay_t<std::remove_cvref_t<decltype(arg)>>;
                if constexpr (std::is_same_v<T, std::string>)
                {
                    auto qtString = QString::fromStdString(arg);
                    this->backingString = qtString;
                    return qtString;
                }
                else if constexpr (std::is_same_v<T, QString>)
                {
                    return arg;
                }
                else
                {
                    static_assert(!type_name<T>().data(),
                                  "unknown type in variant");
                    static_assert(type_name<T>().data(),
                                  "unknown type in variant");
                    static_assert(false, "unknown type in variant");
                }
            },
            this->backingString);
    }

private:
    mutable std::variant<std::string, QString> backingString;
};

boost::json::result_for<String, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<String>, const boost::json::value &jvRoot);

}  // namespace chatterino::eventsub::lib
