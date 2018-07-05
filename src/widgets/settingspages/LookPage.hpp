#pragma once

#include "common/Channel.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QScrollArea>
#include <pajlada/signals/signalholder.hpp>

class QVBoxLayout;

namespace chatterino {

class LookPage : public SettingsPage
{
public:
    LookPage();

    QLayout *createThemeColorChanger();
    QLayout *createFontChanger();
    QLayout *createUiScaleSlider();

    ChannelPtr createPreviewChannel();

    std::vector<pajlada::Signals::ScopedConnection> connections_;
};

}  // namespace chatterino
