#include "ExternalToolsPage.hpp"

#include "Application.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "util/RemoveScrollAreaBackground.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#define STREAMLINK_QUALITY \
    "Choose", "Source", "High", "Medium", "Low", "Audio only"

namespace chatterino {

ExternalToolsPage::ExternalToolsPage()
{
    LayoutCreator<ExternalToolsPage> layoutCreator(this);

    auto scroll = layoutCreator.emplace<QScrollArea>();
    auto widget = scroll.emplaceScrollAreaWidget();
    removeScrollAreaBackground(scroll.getElement(), widget.getElement());

    auto layout = widget.setLayoutType<QVBoxLayout>();

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
            formatRichNamedLink("https://streamlink.github.io/", "Website") +
            " " +
            formatRichNamedLink(
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

    {
        auto group = layout.emplace<QGroupBox>("Custom URI Scheme");
        auto groupLayout = group.setLayoutType<QFormLayout>();

        const auto description = new QLabel(
            "You can open video streams directly in any video player that "
            "has built-in Twitch support and has own URI Scheme.\nE.g.: "
            "IINA for macOS and Potplayer (with extension) for Windows.");
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");

        groupLayout->setWidget(0, QFormLayout::SpanningRole, description);

        groupLayout->addRow("URI Scheme:", this->createLineEdit(
                                               getSettings()->customURIScheme));
    }

    {
        auto group = layout.emplace<QGroupBox>("Image Uploader");
        auto groupLayout = group.setLayoutType<QFormLayout>();

        const auto description =
            new QLabel("You can set custom host for uploading images, like "
                       "imgur.com or s-ul.eu.<br>Check " +
                       formatRichNamedLink("https://gist.github.com/zneix/"
                                           "14d828c145f755394e5ab0301c314c2a",
                                           "this guide") +
                       " for help.");
        description->setWordWrap(true);
        description->setStyleSheet("color: #bbb");
        description->setTextFormat(Qt::RichText);
        description->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                             Qt::LinksAccessibleByKeyboard |
                                             Qt::LinksAccessibleByKeyboard);
        description->setOpenExternalLinks(true);

        groupLayout->setWidget(0, QFormLayout::SpanningRole, description);

        groupLayout->addRow(
            "Request URL: ",
            this->createLineEdit(getSettings()->imageUploaderUrl));
        groupLayout->addRow(
            "Form field: ",
            this->createLineEdit(getSettings()->imageUploaderFormField));
        groupLayout->addRow(
            "Extra Headers: ",
            this->createLineEdit(getSettings()->imageUploaderHeaders));
        groupLayout->addRow(
            "Image link: ",
            this->createLineEdit(getSettings()->imageUploaderLink));
        groupLayout->addRow(
            "Deletion link: ",
            this->createLineEdit(getSettings()->imageUploaderDeletionLink));
    }

    layout->addStretch(1);
}

}  // namespace chatterino
