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

private:
    void initializeUi();

    void addInterfaceTab(LayoutCreator<QVBoxLayout> layout);
    void addMessageTab(LayoutCreator<QVBoxLayout> layout);
    void addEmoteTab(LayoutCreator<QVBoxLayout> layout);
    void addSplitHeaderTab(LayoutCreator<QVBoxLayout> layout);
    void addBadgesTab(LayoutCreator<QVBoxLayout> layout);

    void addLastReadMessageIndicatorPatternSelector(
        LayoutCreator<QVBoxLayout> layout);

    QLayout *createThemeColorChanger();
    QLayout *createFontChanger();
    QLayout *createBoldScaleSlider();

    ChannelPtr createPreviewChannel();

    std::vector<pajlada::Signals::ScopedConnection> connections_;
};

}  // namespace chatterino
