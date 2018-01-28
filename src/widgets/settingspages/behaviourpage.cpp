#include "behaviourpage.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QVBoxLayout>

#include <util/layoutcreator.hpp>

#define WINDOW_TOPMOST "Window always on top (requires restart)"
#define INPUT_EMPTY "Hide input box when empty"
#define LAST_MSG "Show last read message indicator (marks the spot where you left the window)"
#define PAUSE_HOVERING "When hovering"

#define STREAMLINK_QUALITY "Choose", "Source", "High", "Medium", "Low", "Audio only"

namespace chatterino {
namespace widgets {
namespace settingspages {
BehaviourPage::BehaviourPage()
    : SettingsPage("Behaviour", ":/images/behave.svg")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<BehaviourPage> layoutCreator(this);

    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    auto form = layout.emplace<QFormLayout>().withoutMargin();
    {
        form->addRow("Window:", this->createCheckBox(WINDOW_TOPMOST, settings.windowTopMost));
        form->addRow("Messages:", this->createCheckBox(INPUT_EMPTY, settings.hideEmptyInput));
        form->addRow("", this->createCheckBox(LAST_MSG, settings.showLastMessageIndicator));
        form->addRow("Pause chat:", this->createCheckBox(PAUSE_HOVERING, settings.pauseChatHover));

        form->addRow("Mouse scroll speed:", this->createMouseScrollSlider());
        form->addRow("Links:", this->createCheckBox("Open links only on double click",
                                                    settings.linksDoubleClickOnly));
    }

    layout->addSpacing(16);

    auto group = layout.emplace<QGroupBox>("Streamlink");
    {
        auto groupLayout = group.setLayoutType<QFormLayout>();
        groupLayout->addRow("Streamlink path:", this->createLineEdit(settings.streamlinkPath));
        groupLayout->addRow("Prefered quality:",
                            this->createComboBox({STREAMLINK_QUALITY}, settings.preferredQuality));
        groupLayout->addRow("Additional options:", this->createLineEdit(settings.streamlinkOpts));
    }

    layout->addStretch(1);
}

QSlider *BehaviourPage::createMouseScrollSlider()
{
    QSlider *slider = new QSlider(Qt::Horizontal);

    float currentValue = singletons::SettingManager::getInstance().mouseScrollMultiplier;
    int sliderValue = ((currentValue - 0.1f) / 2.f) * 99.f;
    slider->setValue(sliderValue);

    QObject::connect(slider, &QSlider::valueChanged, [](int newValue) {
        float mul = static_cast<float>(newValue) / 99.f;
        float newSliderValue = (mul * 2.1f) + 0.1f;
        singletons::SettingManager::getInstance().mouseScrollMultiplier = newSliderValue;
    });

    return slider;
}
}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
