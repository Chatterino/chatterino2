#include "FeelPage.hpp"

#include "Application.hpp"
#include "util/LayoutCreator.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#define PAUSE_HOVERING "When hovering"
#define SCROLL_SMOOTH "Smooth scrolling"
#define SCROLL_NEWMSG "Smooth scrolling for new messages"

#define LIMIT_CHATTERS_FOR_SMALLER_STREAMERS \
    "Only fetch chatters list for viewers under X viewers"

namespace chatterino {

FeelPage::FeelPage()
    : SettingsPage("Feel", ":/settings/behave.svg")
{
    auto app = getApp();
    LayoutCreator<FeelPage> layoutCreator(this);

    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    // layout.append(this->createCheckBox("Use a seperate write connection.",
    //                                    getSettings()->twitchSeperateWriteConnection));
    layout.append(this->createCheckBox(SCROLL_SMOOTH,
                                       getSettings()->enableSmoothScrolling));
    layout.append(this->createCheckBox(
        SCROLL_NEWMSG, getSettings()->enableSmoothScrollingNewMessages));

    auto form = layout.emplace<QFormLayout>().withoutMargin();
    {
        form->addRow(
            "", this->createCheckBox(
                    "Show which users joined the channel (up to 1000 chatters)",
                    app->settings->showJoins));
        form->addRow(
            "", this->createCheckBox(
                    "Show which users parted the channel (up to 1000 chatters)",
                    app->settings->showParts));

        form->addRow("Pause chat:",
                     this->createCheckBox(PAUSE_HOVERING,
                                          app->settings->pauseChatHover));

        form->addRow("Mouse scroll speed:", this->createMouseScrollSlider());
        form->addRow("Links:",
                     this->createCheckBox("Open links only on double click",
                                          app->settings->linksDoubleClickOnly));
    }

    layout->addSpacing(16);

    {
        auto group = layout.emplace<QGroupBox>("Auto-completion");
        auto groupLayout = group.setLayoutType<QFormLayout>();
        groupLayout->addRow(
            LIMIT_CHATTERS_FOR_SMALLER_STREAMERS,
            this->createCheckBox(
                "", app->settings->onlyFetchChattersForSmallerStreamers));

        groupLayout->addRow(
            "What viewer count counts as a \"smaller streamer\"",
            this->createSpinBox(app->settings->smallStreamerLimit, 10, 50000));
    }

    {
        auto group = layout.emplace<QGroupBox>("Misc");
        auto groupLayout = group.setLayoutType<QVBoxLayout>();

        groupLayout.append(this->createCheckBox("Show whispers inline",
                                                app->settings->inlineWhispers));
    }

    layout->addStretch(1);
}

QSlider *FeelPage::createMouseScrollSlider()
{
    auto app = getApp();
    auto slider = new QSlider(Qt::Horizontal);

    float currentValue = app->settings->mouseScrollMultiplier;
    int sliderValue = int(((currentValue - 0.1f) / 2.f) * 99.f);
    slider->setValue(sliderValue);

    QObject::connect(slider, &QSlider::valueChanged, [=](int newValue) {
        float mul = static_cast<float>(newValue) / 99.f;
        float newSliderValue = (mul * 2.1f) + 0.1f;
        app->settings->mouseScrollMultiplier = newSliderValue;
    });

    return slider;
}

}  // namespace chatterino
