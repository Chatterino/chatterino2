#pragma once

#include "widgets/settingspages/settingspage.hpp"

#include <QSlider>

namespace chatterino {
namespace widgets {
namespace settingspages {

class AppearancePage : public SettingsPage
{
public:
    AppearancePage();

    QLayout *createThemeColorChanger();
    QLayout *createFontChanger();
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
