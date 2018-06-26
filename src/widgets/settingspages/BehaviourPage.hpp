#pragma once

#include <QSlider>

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class BehaviourPage : public SettingsPage
{
public:
    BehaviourPage();

private:
    QSlider *createMouseScrollSlider();
};

}  // namespace chatterino
