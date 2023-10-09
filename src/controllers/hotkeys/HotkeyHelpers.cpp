#include "controllers/hotkeys/HotkeyHelpers.hpp"

#include "controllers/hotkeys/ActionNames.hpp"
#include "controllers/hotkeys/HotkeyCategory.hpp"

#include <QStringList>

#include <optional>

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

std::optional<ActionDefinition> findHotkeyActionDefinition(
    HotkeyCategory category, const QString &action)
{
    auto allActions = actionNames.find(category);
    if (allActions != actionNames.end())
    {
        const auto &actionsMap = allActions->second;
        auto definition = actionsMap.find(action);
        if (definition != actionsMap.end())
        {
            return {definition->second};
        }
    }
    return {};
}

}  // namespace chatterino
