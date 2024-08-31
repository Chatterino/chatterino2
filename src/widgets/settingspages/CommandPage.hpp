#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QTimer>

namespace chatterino {

class CommandPage : public SettingsPage
{
public:
    CommandPage();

private:
    QTimer commandsEditTimer_;
};

}  // namespace chatterino
