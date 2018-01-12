#pragma once

#include <QSlider>

#include "widgets/settingspages/settingspage.hpp"

namespace chatterino {
namespace widgets {
namespace settingspages {

class BehaviourPage : public SettingsPage
{
public:
    BehaviourPage();

private:
    QSlider *createMouseScrollSlider();
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
