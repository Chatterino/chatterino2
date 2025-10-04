#pragma once

#include <memory>

namespace chatterino {

namespace functional::detail {

template <typename T>
std::weak_ptr<T> asWeak(std::weak_ptr<T> weak)
{
    return std::move(weak);
}

template <typename T>
std::weak_ptr<T> asWeak(const std::shared_ptr<T> &strong)
{
    return {strong};
}

}  // namespace functional::detail

decltype(auto) weakGuarded(auto &&cb, auto &&...weaks)
{
    return [... weaks{functional::detail::asWeak(
                std::forward<decltype(weaks)>(weaks))},
            cb{std::forward<decltype(cb)>(cb)}](auto &&...args) mutable {
        auto strongs = std::make_tuple(weaks.lock()...);

        [&]<size_t... I>(std::index_sequence<I...>) {
            if ((std::get<I>(strongs) && ...))
            {
                cb(std::get<I>(std::move(strongs))...,
                   std::forward<decltype(args)>(args)...);
            }
        }(std::make_index_sequence<sizeof...(weaks)>());
    };
}

}  // namespace chatterino
