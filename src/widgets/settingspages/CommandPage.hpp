#pragma once

#include <QTextEdit>
#include <QTimer>

#include "widgets/settingspages/settingspage.hpp"

namespace chatterino {
namespace widgets {
namespace settingspages {

class CommandPage : public SettingsPage
{
public:
    CommandPage();

private:
    QTimer commandsEditTimer;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
