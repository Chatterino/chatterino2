#pragma once

#include <QTimer>

#include "widgets/settingspages/SettingsPage.hpp"

class QPushButton;

namespace chatterino {
namespace widgets {
namespace settingspages {

class ModerationPage : public SettingsPage
{
public:
    ModerationPage();

private:
    QTimer itemsChangedTimer;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
