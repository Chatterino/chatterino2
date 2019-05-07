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
    QComboBox *createToastReactionComboBox(
        std::vector<pajlada::Signals::ScopedConnection> managedConnections);
};

}  // namespace chatterino
