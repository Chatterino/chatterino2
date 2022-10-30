#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

class QPushButton;
class QListWidget;

class QVBoxLayout;

namespace chatterino {

class NotificationPage : public SettingsPage
{
public:
    NotificationPage();

private:
    QComboBox *createToastReactionComboBox();
};

}  // namespace chatterino
