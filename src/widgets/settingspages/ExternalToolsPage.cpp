#include "ExternalToolsPage.hpp"

#include "Application.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"

#include <QGroupBox>

#define STREAMLINK_QUALITY \
    "Choose", "Source", "High", "Medium", "Low", "Audio only"

namespace chatterino {

ExternalToolsPage::ExternalToolsPage()
    : SettingsPage("External tools", "")
{
    auto app = getApp();

    LayoutCreator<ExternalToolsPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    {
        auto group = layout.emplace<QGroupBox>("Streamlink");
        auto groupLayout = group.setLayoutType<QFormLayout>();

        auto description = new QLabel(
            "Streamlink is a command-line utility that pipes video streams "
            "from various "
            "services into a video player, such as VLC. Make sure to edit the "
            "configuration file before you use it!");
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");

        auto links = new QLabel(
            createNamedLink("https://streamlink.github.io/", "Website") + " " +
            createNamedLink(
                "https://github.com/streamlink/streamlink/releases/latest",
                "Download"));
        links->setTextFormat(Qt::RichText);
        links->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                       Qt::LinksAccessibleByKeyboard |
                                       Qt::LinksAccessibleByKeyboard);
        links->setOpenExternalLinks(true);

        groupLayout->setWidget(0, QFormLayout::SpanningRole, description);
        groupLayout->setWidget(1, QFormLayout::SpanningRole, links);

        auto customPathCb =
            this->createCheckBox("Use custom path (Enable if using "
                                 "non-standard streamlink installation path)",
                                 getSettings()->streamlinkUseCustomPath);
        groupLayout->setWidget(2, QFormLayout::SpanningRole, customPathCb);

        auto customPath = this->createLineEdit(getSettings()->streamlinkPath);
        customPath->setPlaceholderText(
            "Path to folder where Streamlink executable can be found");
        groupLayout->addRow("Custom streamlink path:", customPath);
        groupLayout->addRow(
            "Preferred quality:",
            this->createComboBox({STREAMLINK_QUALITY},
                                 getSettings()->preferredQuality));
        groupLayout->addRow(
            "Additional options:",
            this->createLineEdit(getSettings()->streamlinkOpts));

        getSettings()->streamlinkUseCustomPath.connect(
            [=](const auto &value, auto) {
                customPath->setEnabled(value);  //
            },
            this->managedConnections_);
    }

    layout->addStretch(1);
}

}  // namespace chatterino
