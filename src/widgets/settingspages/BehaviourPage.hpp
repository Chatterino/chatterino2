#pragma once

#include <QSlider>

#include "widgets/settingspages/SettingsPage.hpp"

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
