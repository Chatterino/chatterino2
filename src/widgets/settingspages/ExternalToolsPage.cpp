// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/settingspages/ExternalToolsPage.hpp"

#include "controllers/spellcheck/SpellChecker.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/Clipboard.hpp"
#include "util/Helpers.hpp"
#include "util/ImageUploader.hpp"
#include "util/StreamLink.hpp"
#include "widgets/settingspages/SettingWidget.hpp"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

#include <algorithm>

namespace chatterino {

namespace {

inline const QStringList STREAMLINK_QUALITY = {
    "Choose", "Source", "High", "Medium", "Low", "Audio only",
};

void exportImageUploaderSettings(QWidget *parent)
{
    const auto &s = *getSettings();

    QJsonObject settingsObj = imageuploader::detail::exportSettings(s);
    QJsonDocument doc(settingsObj);
    crossPlatformCopy(doc.toJson(QJsonDocument::Indented));

    QMessageBox::information(
        parent, "Settings Exported",
        "Image uploader settings have been copied to clipboard as JSON.");
}

void importImageUploaderSettings(QWidget *parent)
{
    QString clipboardText = getClipboardText().trimmed();

    auto res = imageuploader::detail::validateImportJson(clipboardText);
    if (!res)
    {
        QMessageBox::warning(
            parent, "Import Failed",
            QString("Error validating image uploader import: %1.")
                .arg(res.error()));
        return;
    }
    const auto &settingsObj = *res;

    int ret = QMessageBox::question(
        parent, "Import Settings",
        "This will overwrite your current image uploader settings. Continue?",
        QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes)
    {
        return;
    }

    auto &s = *getSettings();
    if (imageuploader::detail::importSettings(settingsObj, s))
    {
        QMessageBox::information(
            parent, "Import Successful",
            "Image uploader settings have been imported successfully!");
    }
    else
    {
        QMessageBox::warning(
            parent, "Import Failed",
            "No valid image uploader settings found in the JSON.");
    }
}

}  // namespace

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

    this->initLayout(*this->view);
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

        layout.addDescription(
            "Export your current image uploader settings as JSON to share with "
            "others, or import settings from clipboard (compatible with ShareX "
            ".sxcu format).");

        auto *buttonLayout = new QHBoxLayout;

        auto *importButton = new QPushButton("Import Settings from Clipboard");
        importButton->setToolTip(
            "Import image uploader settings from clipboard JSON");
        QObject::connect(importButton, &QPushButton::clicked, [this]() {
            importImageUploaderSettings(this);
        });
        buttonLayout->addWidget(importButton);

        auto *exportButton = new QPushButton("Export Settings to Clipboard");
        exportButton->setToolTip(
            "Copy current image uploader settings to clipboard as JSON");
        QObject::connect(exportButton, &QPushButton::clicked, [this]() {
            exportImageUploaderSettings(this);
        });
        buttonLayout->addWidget(exportButton);

        buttonLayout->addStretch();
        layout.addLayout(buttonLayout);
    }

#ifdef CHATTERINO_WITH_SPELLCHECK
    {
        // auto *form = new QFormLayout;
        layout.addTitle("Spell checker (experimental)");

        layout.addDescription(
            u"Check the spelling of words in the input box of splits."
            " Chatterino does not include dictionaries - they have to "
            "be downloaded or created manually. Chatterino expects "
            "Hunspell "
            "dictionaries in " %
            formatRichNamedLink(getApp()->getPaths().dictionariesDirectory,
                                getApp()->getPaths().dictionariesDirectory) %
            u". The file index.aff has to contain the affixes and "
            u"index.dic "
            u"must contain the dictionary (subject to change).");

        SettingWidget::checkbox("Check spelling by default",
                                s.enableSpellChecking)
            ->setTooltip("Check the spelling of words in the input box of all "
                         "splits by default.")
            ->addTo(layout);

        auto toItem =
            [](const DictionaryInfo &dict) -> std::pair<QString, QVariant> {
            return {
                dict.name,
                dict.path,
            };
        };
        std::vector<std::pair<QString, QVariant>> dictList{{"None", ""}};

        std::ranges::transform(
            getApp()->getSpellChecker()->getSystemDictionaries(),
            std::back_inserter(dictList), toItem);

        if (dictList.size() > 1)
        {
            SettingWidget::dropdown(
                "Fallback spellchecking dictionary (requires restart)",
                s.spellCheckingFallback, dictList)
                ->addTo(layout);
        }
    }
#endif

    layout.addStretch();
}

}  // namespace chatterino
