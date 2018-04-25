#include "externaltoolspage.hpp"

#include "util/layoutcreator.hpp"

#include <QGroupBox>

#define STREAMLINK_QUALITY "Choose", "Source", "High", "Medium", "Low", "Audio only"

namespace chatterino {
namespace widgets {
namespace settingspages {

ExternalToolsPage::ExternalToolsPage()
    : SettingsPage("External tools", "")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<ExternalToolsPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>().withoutMargin();

    {
        auto group = layout.emplace<QGroupBox>("Streamlink");
        auto groupLayout = group.setLayoutType<QFormLayout>();
        groupLayout->addRow("Streamlink path:", this->createLineEdit(settings.streamlinkPath));
        groupLayout->addRow("Prefered quality:",
                            this->createComboBox({STREAMLINK_QUALITY}, settings.preferredQuality));
        groupLayout->addRow("Additional options:", this->createLineEdit(settings.streamlinkOpts));
    }
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
