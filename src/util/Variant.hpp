#pragma once

namespace chatterino::variant {

/// Compile-time safe visitor for std and boost variants.
///
/// From https://en.cppreference.com/w/cpp/utility/variant/visit
///
/// Usage:
///
/// ```
/// std::variant<int, double> v;
/// std::visit(variant::Overloaded{
///     [](double) { qDebug() << "double"; },
///     [](int) { qDebug() << "int"; }
/// }, v);
/// ```
template <class... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

// Technically, we shouldn't need this, as we're on C++ 20,
// but not all of our compilers support CTAD for aggregates yet.
template <class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace chatterino::variant
