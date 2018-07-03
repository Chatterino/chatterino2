#include "BehaviourPage.hpp"

#include "Application.hpp"
#include "util/LayoutCreator.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#ifdef USEWINSDK
#define WINDOW_TOPMOST "Window always on top"
#else
#define WINDOW_TOPMOST "Window always on top (requires restart)"
#endif
#define INPUT_EMPTY "Hide input box when empty"
#define PAUSE_HOVERING "When hovering"
#define LAST_MSG "Mark the last message you read (dotted line)"

#define LIMIT_CHATTERS_FOR_SMALLER_STREAMERS "Only fetch chatters list for viewers under X viewers"

namespace chatterino {

BehaviourPage::BehaviourPage()
    : SettingsPage("Feel", ":/images/behave.svg")
{
    auto app = getApp();
    LayoutCreator<BehaviourPage> layoutCreator(this);

    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    auto form = layout.emplace<QFormLayout>().withoutMargin();
    {
        form->addRow("Window:", this->createCheckBox(WINDOW_TOPMOST, app->settings->windowTopMost));
        form->addRow("Messages:", this->createCheckBox(INPUT_EMPTY, app->settings->hideEmptyInput));
        form->addRow(
            "", this->createCheckBox("Show which users joined the channel (up to 1000 chatters)",
                                     app->settings->showJoins));
        form->addRow(
            "", this->createCheckBox("Show which users parted the channel (up to 1000 chatters)",
                                     app->settings->showParts));

        form->addRow("", this->createCheckBox("Show message length while typing",
                                              getSettings()->showMessageLength));
        form->addRow("", this->createCheckBox(LAST_MSG, getSettings()->showLastMessageIndicator));
        {
            auto *combo = new QComboBox(this);
            combo->addItems({"Dotted", "Solid"});

            const auto currentIndex = []() -> int {
                switch (getApp()->settings->lastMessagePattern.getValue()) {
                    case Qt::SolidLine: {
                        return 1;
                    }
                    default:
                    case Qt::VerPattern: {
                        return 0;
                    }
                }
            }();
            combo->setCurrentIndex(currentIndex);

            QObject::connect(combo,
                             static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                             [](int index) {
                                 Qt::BrushStyle brush;
                                 switch (index) {
                                     case 1:
                                         brush = Qt::SolidPattern;
                                         break;
                                     default:
                                     case 0:
                                         brush = Qt::VerPattern;
                                         break;
                                 }
                                 getSettings()->lastMessagePattern = brush;
                             });

            auto hbox = form.emplace<QHBoxLayout>().withoutMargin();
            hbox.emplace<QLabel>("Last message indicator pattern");
            hbox.append(combo);
        }

        form->addRow("Pause chat:",
                     this->createCheckBox(PAUSE_HOVERING, app->settings->pauseChatHover));

        form->addRow("Mouse scroll speed:", this->createMouseScrollSlider());
        form->addRow("Links:", this->createCheckBox("Open links only on double click",
                                                    app->settings->linksDoubleClickOnly));
    }

    layout->addSpacing(16);

    {
        auto group = layout.emplace<QGroupBox>("Auto-completion");
        auto groupLayout = group.setLayoutType<QFormLayout>();
        groupLayout->addRow(
            LIMIT_CHATTERS_FOR_SMALLER_STREAMERS,
            this->createCheckBox("", app->settings->onlyFetchChattersForSmallerStreamers));

        groupLayout->addRow("What viewer count counts as a \"smaller streamer\"",
                            this->createSpinBox(app->settings->smallStreamerLimit, 10, 50000));
    }

    {
        auto group = layout.emplace<QGroupBox>("Misc");
        auto groupLayout = group.setLayoutType<QVBoxLayout>();

        groupLayout.append(
            this->createCheckBox("Show whispers inline", app->settings->inlineWhispers));
    }

    layout->addStretch(1);
}

QSlider *BehaviourPage::createMouseScrollSlider()
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
