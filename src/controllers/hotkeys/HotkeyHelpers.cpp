#include "controllers/hotkeys/HotkeyHelpers.hpp"

#include <QStringList>

namespace chatterino {

std::vector<QString> parseHotkeyArguments(QString argumentString)
{
    std::vector<QString> arguments;

    argumentString = argumentString.trimmed();

    if (argumentString.isEmpty())
    {
        // argumentString is empty, early out to ensure we don't end up with a vector with one empty element
        return arguments;
    }

    auto argList = argumentString.split("\n");

    // convert the QStringList to our preferred std::vector
    for (const auto &arg : argList)
    {
        arguments.push_back(arg.trimmed());
    }

    return arguments;
}

}  // namespace chatterino
