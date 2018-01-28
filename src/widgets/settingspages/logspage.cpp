#include "logspage.hpp"

#include <QFormLayout>
#include <QVBoxLayout>

#include "util/layoutcreator.hpp"

namespace chatterino {
namespace widgets {
namespace settingspages {

LogsPage::LogsPage()
    : SettingsPage("Logs", "")
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();
    util::LayoutCreator<LogsPage> layoutCreator(this);
    auto layout = layoutCreator.emplace<QVBoxLayout>().withoutMargin();

    {
        auto form = layout.emplace<QFormLayout>();

        // clang-format off
        form->addRow("Enabled:",   this->createCheckBox("Enable logging", settings.enableLogging));
        // clang-format on
    }

    layout->addStretch(1);
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
