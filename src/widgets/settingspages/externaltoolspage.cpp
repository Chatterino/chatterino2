#include "externaltoolspage.hpp"

#include "application.hpp"
#include "util/layoutcreator.hpp"

#include <QGroupBox>

#define STREAMLINK_QUALITY "Choose", "Source", "High", "Medium", "Low", "Audio only"

namespace chatterino {
namespace widgets {
namespace settingspages {

ExternalToolsPage::ExternalToolsPage()
    : SettingsPage("External tools", "")
{
    auto app = getApp();

    util::LayoutCreator<ExternalToolsPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    {
        auto group = layout.emplace<QGroupBox>("Streamlink");
        auto groupLayout = group.setLayoutType<QFormLayout>();
        groupLayout->addRow("Streamlink path:",
                            this->createLineEdit(app->settings->streamlinkPath));
        groupLayout->addRow(
            "Prefered quality:",
            this->createComboBox({STREAMLINK_QUALITY}, app->settings->preferredQuality));
        groupLayout->addRow("Additional options:",
                            this->createLineEdit(app->settings->streamlinkOpts));
    }

    layout->addStretch(1);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
