#pragma once

#include "controllers/hotkeys/ActionNames.hpp"

#include <boost/optional/optional.hpp>
#include <QString>

#include <vector>

namespace chatterino {

std::vector<QString> parseHotkeyArguments(QString argumentString);
boost::optional<ActionDefinition> findHotkeyActionDefinition(
    HotkeyCategory category, const QString &action);

}  // namespace chatterino
