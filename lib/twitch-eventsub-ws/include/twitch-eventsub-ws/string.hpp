#pragma once

#include <boost/json.hpp>
#include <QString>

#include <iostream>
#include <string>
#include <variant>

namespace chatterino::eventsub::lib {

struct String {
    explicit String(std::string v)
        : backingString(std::move(v))
    {
    }
    ~String() = default;

    String(const String &s) = delete;
    String(String &&s) = default;

    String &operator=(const String &) = delete;
    String &operator=(String &&) = default;

    QString qt()
    {
        return std::visit(
            [this](auto &&arg) -> QString {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>)
                {
                    std::cerr << "COPYING\n";
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
                    static_assert(false, "unknown type in variant xd");
                }
            },
            this->backingString);
    }

    // private:
    std::variant<std::string, QString> backingString;
};

boost::json::result_for<String, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<String>, const boost::json::value &jvRoot);

}  // namespace chatterino::eventsub::lib
