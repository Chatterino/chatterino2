// SPDX-FileCopyrightText: 2021 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/hotkeys/HotkeyHelpers.hpp"

#include "controllers/hotkeys/ActionNames.hpp"
#include "controllers/hotkeys/HotkeyCategory.hpp"

#include <QKeyCombination>
#include <QStringList>

#include <array>
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

QKeySequence normalizeKeySequence(const QKeySequence &seq)
{
    if (seq.isEmpty())
    {
        return seq;
    }

    // First, check if any normalization is needed
    bool needsNormalization = false;
    for (int i = 0; i < seq.count(); i++)
    {
        if (seq[i].key() == Qt::Key_Enter)
        {
            needsNormalization = true;
            break;
        }
    }

    if (!needsNormalization)
    {
        return seq;
    }

    // Build normalized key combinations, preserving all keys in the sequence
    // QKeySequence supports up to 4 key combinations
    std::array<QKeyCombination, 4> combos{};
    int count = seq.count();

    for (int i = 0; i < count; i++)
    {
        auto combo = seq[i];
        if (combo.key() == Qt::Key_Enter)
        {
            combos.at(i) =
                QKeyCombination(combo.keyboardModifiers(), Qt::Key_Return);
        }
        else
        {
            combos.at(i) = combo;
        }
    }

    // Construct QKeySequence with the appropriate number of keys
    switch (count)
    {
        case 1:
            return {combos.at(0)};
        case 2:
            return {combos.at(0), combos.at(1)};
        case 3:
            return {combos.at(0), combos.at(1), combos.at(2)};
        case 4:
        default:
            return {combos.at(0), combos.at(1), combos.at(2), combos.at(3)};
    }
}

}  // namespace chatterino
