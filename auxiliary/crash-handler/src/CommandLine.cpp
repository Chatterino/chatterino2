#include "CommandLine.hpp"

std::vector<std::wstring> splitEncodedChatterinoArgs(
    const std::wstring &encodedArgs)
{
    std::vector<std::wstring> parts;

    std::wstring_view view(encodedArgs);
    std::wstring_view::size_type pos{};
    std::wstring part;

    while ((pos = view.find(L'+')) != std::wstring_view::npos)
    {
        if (pos + 1 == view.length())  // string ends with +
        {
            parts.emplace_back(std::move(part));
            return parts;
        }

        auto next = view[pos + 1];
        if (next == L'+')  // escaped plus (++)
        {
            part += view.substr(0, pos);
            part.push_back(L'+');
            view = view.substr(pos + 2);
            continue;
        }

        // actual separator
        part += view.substr(0, pos);
        parts.emplace_back(std::move(part));
        part = {};
        view = view.substr(pos + 1);
    }

    if (!view.empty())
    {
        part += view;
    }
    if (!part.empty())
    {
        parts.emplace_back(std::move(part));
    }

    return parts;
}
