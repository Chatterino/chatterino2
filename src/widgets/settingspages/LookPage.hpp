#pragma once

#include "common/Channel.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QScrollArea>
#include <pajlada/signals/signalholder.hpp>

class QVBoxLayout;

namespace chatterino {

class LookPage : public SettingsPage
{
public:
    LookPage();

    void addInterfaceTab(LayoutCreator<QVBoxLayout> layout);
    void addMessageTab(LayoutCreator<QVBoxLayout> layout);
    void addEmoteTab(LayoutCreator<QVBoxLayout> layout);

    QLayout *createThemeColorChanger();
    QLayout *createFontChanger();
    QLayout *createUiScaleSlider();

    ChannelPtr createPreviewChannel();

    std::vector<pajlada::Signals::ScopedConnection> connections_;
};

}  // namespace chatterino
