#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QScrollArea>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {
namespace widgets {
namespace settingspages {

class AppearancePage : public SettingsPage
{
public:
    AppearancePage();

    QLayout *createThemeColorChanger();
    QLayout *createFontChanger();
    QLayout *createUiScaleSlider();

    std::vector<pajlada::Signals::ScopedConnection> connections_;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
