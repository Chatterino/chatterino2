#pragma once

#include <string_view>

namespace chatterino {

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

}  // namespace chatterino
