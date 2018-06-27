#pragma once

#include <QTimer>

#include "widgets/settingspages/SettingsPage.hpp"

class QPushButton;

namespace chatterino {

class ModerationPage : public SettingsPage
{
public:
    ModerationPage();

private:
    QTimer itemsChangedTimer;
};

}  // namespace chatterino
