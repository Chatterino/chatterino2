#pragma once

#include "controllers/hotkeys/ActionNames.hpp"

#include <QString>

#include <optional>
#include <vector>

namespace chatterino {

std::vector<QString> parseHotkeyArguments(QString argumentString);
std::optional<ActionDefinition> findHotkeyActionDefinition(
    HotkeyCategory category, const QString &action);

}  // namespace chatterino
