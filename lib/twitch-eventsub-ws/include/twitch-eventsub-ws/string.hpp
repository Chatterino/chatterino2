#pragma once

#include <boost/json.hpp>
#include <QString>

#include <string>
#include <variant>

namespace chatterino::eventsub::lib {

template <class>
constexpr bool DEPENDENT_FALSE = false;  // workaround before CWG2518/P2593R1

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
                    static_assert(DEPENDENT_FALSE<T>,
                                  "unknown type in variant");
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
