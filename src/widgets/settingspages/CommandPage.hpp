#pragma once

#include <QTextEdit>
#include <QTimer>

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class CommandPage : public SettingsPage
{
public:
    CommandPage();

private:
    QTimer commandsEditTimer_;
};

}  // namespace chatterino
