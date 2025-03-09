#include "widgets/settingspages/ExternalToolsPage.hpp"

#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/StreamLink.hpp"
#include "widgets/settingspages/SettingWidget.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>

namespace chatterino {

inline const QStringList STREAMLINK_QUALITY = {
    "Choose", "Source", "High", "Medium", "Low", "Audio only",
};

ExternalToolsPage::ExternalToolsPage()
    : view(GeneralPageView::withoutNavigation(this))
{
    auto *y = new QVBoxLayout;
    auto *x = new QHBoxLayout;
    x->addWidget(this->view);
    auto *z = new QFrame;
    z->setLayout(x);
    y->addWidget(z);
    this->setLayout(y);

    this->initLayout(*view);
}

bool ExternalToolsPage::filterElements(const QString &query)
{
    if (this->view)
    {
        return this->view->filterElements(query) || query.isEmpty();
    }

    return false;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void ExternalToolsPage::initLayout(GeneralPageView &layout)
{
    auto &s = *getSettings();

    {
        auto *form = new QFormLayout;
        layout.addTitle("Streamlink");
        layout.addDescription("Streamlink is a command-line utility that pipes "
                              "video streams from "
                              "various services into a video player, such as "
                              "VLC. Make sure to edit "
                              "the configuration file before you use it!");
        layout.addDescription(
            formatRichNamedLink("https://streamlink.github.io/", "Website") +
            " " +
            formatRichNamedLink(
                "https://github.com/streamlink/streamlink/releases/latest",
                "Download") +
            " " +
            formatRichNamedLink("https://streamlink.github.io/cli.html#twitch",
                                "Documentation"));

        SettingWidget::checkbox("Use custom path (Enable if using non-standard "
                                "streamlink installation path)",
                                s.streamlinkUseCustomPath)
            ->addTo(layout);

        layout.addDescription(
            QStringLiteral(
                "Chatterino expects the executable to be called \"%1\".")
                .arg(STREAMLINK_BINARY_NAME));

        layout.addLayout(form);

        SettingWidget::lineEdit(
            "Custom streamlink path", s.streamlinkPath,
            "Path to folder where Streamlink executable can be found")
            ->conditionallyEnabledBy(s.streamlinkUseCustomPath)
            ->addTo(layout, form);

        SettingWidget::dropdown("Preferred quality", s.preferredQuality)
            ->addTo(layout, form);

        SettingWidget::lineEdit("Additional options", s.streamlinkOpts, "")
            ->addTo(layout, form);
    }

    {
        layout.addTitle("Custom stream player");
        layout.addDescription(
            "You can open Twitch streams directly in any video player that has "
            "built-in Twitch support and has own URI Scheme.\nE.g.: IINA for "
            "macOS and Potplayer (with extension) for Windows.\n\nWith this "
            "value set, you will get the option to \"Open in custom player\" "
            "when right-clicking a channel header.");

        SettingWidget::lineEdit("Custom stream player URI Scheme",
                                s.customURIScheme, "custom-player-scheme://")
            ->addTo(layout);
    }

    {
        auto *form = new QFormLayout;
        layout.addTitle("Image Uploader");

        layout.addDescription(
            "You can set custom host for uploading images, like imgur.com or "
            "s-ul.eu.<br>Check " +
            formatRichNamedLink("https://chatterino.com/help/image-uploader",
                                "this guide") +
            " for help.");

        SettingWidget::checkbox("Enable image uploader", s.imageUploaderEnabled)
            ->addTo(layout);

        SettingWidget::checkbox("Ask for confirmation when uploading an image",
                                s.askOnImageUpload)
            ->addTo(layout);

        layout.addLayout(form);

        SettingWidget::lineEdit("Request URL", s.imageUploaderUrl)
            ->addTo(layout, form);

        SettingWidget::lineEdit("Form field", s.imageUploaderFormField)
            ->addTo(layout, form);

        SettingWidget::lineEdit("Extra Headers", s.imageUploaderHeaders)
            ->addTo(layout, form);

        SettingWidget::lineEdit("Image link", s.imageUploaderLink)
            ->addTo(layout, form);

        SettingWidget::lineEdit("Deletion link", s.imageUploaderDeletionLink)
            ->addTo(layout, form);
    }

    layout.addStretch();
}

}  // namespace chatterino
