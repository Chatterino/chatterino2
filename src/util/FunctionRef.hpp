// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace chatterino {

/// A non-owning, type-erased reference to a callable. Callables can be lambdas,
/// functions, or std::functions.
///
/// It is intended to be used for functions taking a callback that is called
/// immediately (such as filter/map functions). Since this doesn't own the
/// callable, it's not safe to store.
///
/// This is based on llvm::function_ref (updated for C++ 20).
template <typename Fn>
class FunctionRef;

template <typename Ret, typename... Params>
class FunctionRef<Ret(Params...)>
{
public:
    FunctionRef() = default;
    FunctionRef(std::nullptr_t)
    {
    }

    template <typename Callable>
    FunctionRef(Callable &&callable)  // NOLINT
        requires
        // This is not the copy-constructor.
        (!std::same_as<std::remove_cvref_t<Callable>, FunctionRef>) &&
            // Functor must be callable and return a suitable type.
            (std::is_void_v<Ret> ||
             std::convertible_to<
                 decltype(std::declval<Callable>()(std::declval<Params>()...)),
                 Ret>)
        : callback(callTrampoline<std::remove_reference_t<Callable>>)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        , callable(reinterpret_cast<uintptr_t>(&callable))
    {
    }

    Ret operator()(Params... params) const
    {
        return this->callback(this->callable, std::forward<Params>(params)...);
    }

    explicit operator bool() const
    {
        return this->callback != nullptr;
    }

    bool operator==(const FunctionRef &other) const
    {
        return this->callback == other.callback &&
               this->callable == other.callable;
    }

    bool operator!=(const FunctionRef &other) const = default;

private:
    // same signature as callTrampoline
    using Callback = Ret(uintptr_t, Params...);

    /// Pointer to the call trampoline that, given the callable, calls the target function.
    Callback *callback = nullptr;
    /// Pointer to the actual function
    uintptr_t callable = 0;

    template <typename Callable>
    static Ret callTrampoline(uintptr_t callable, Params... params)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return (*reinterpret_cast<Callable *>(callable))(
            std::forward<Params>(params)...);
    }
};

}  // namespace chatterino
