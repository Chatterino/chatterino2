#pragma once

#include <QSlider>

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino
{
    class FeelPage : public SettingsPage
    {
    public:
        FeelPage();

    private:
        QSlider* createMouseScrollSlider();
    };

}  // namespace chatterino
