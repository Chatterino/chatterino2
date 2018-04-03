#pragma once

#include "widgets/settingspages/settingspage.hpp"

class QLabel;

namespace chatterino {
namespace widgets {
namespace settingspages {

class AboutPage : public SettingsPage
{
public:
    AboutPage();

private:
    QLabel *logo;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
