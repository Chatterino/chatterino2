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

}  // namespace chatterino::variant
