#include "behaviourpage.hpp"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <util/layoutcreator.hpp>

#define WINDOW_TOPMOST "Window always on top (requires restart)"
#define INPUT_EMPTY "Hide input box when empty"
#define LAST_MSG "Show last read message indicator"
#define PAUSE_HOVERING "When hovering"

#define STREAMLINK_QUALITY "Source", "High", "Medium", "Low", "Audio only"

namespace chatterino {
namespace widgets {
namespace settingspages {
BehaviourPage::BehaviourPage()
    : SettingsPage("Behaviour", ":/images/behave.svg")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<BehaviourPage> layoutCreator(this);

    auto form = layoutCreator.emplace<QFormLayout>().withoutMargin();
    {
        form->addRow("Window:", this->createCheckBox(WINDOW_TOPMOST, settings.windowTopMost));
        form->addRow("Messages:", this->createCheckBox(INPUT_EMPTY, settings.hideEmptyInput));
        form->addRow("", this->createCheckBox(LAST_MSG, settings.showLastMessageIndicator));
        form->addRow("Pause chat:", this->createCheckBox(PAUSE_HOVERING, settings.pauseChatHover));

        form->addRow("Mouse scroll speed:", this->createMouseScrollSlider());
        form->addRow("Streamlink path:", this->createLineEdit(settings.streamlinkPath));
        form->addRow("Prefered quality:",
                     this->createComboBox({STREAMLINK_QUALITY}, settings.preferredQuality));
    }
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
