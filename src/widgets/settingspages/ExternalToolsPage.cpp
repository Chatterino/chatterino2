#include "widgets/settingspages/ExternalToolsPage.hpp"

#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"
#include "util/StreamLink.hpp"
#include "widgets/settingspages/SettingWidget.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

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

       layout.addDescription(
           "Export your current image uploader settings as JSON to share with "
           "others, or import settings from clipboard (compatible with ShareX .sxcu format).");

       auto *buttonLayout = new QHBoxLayout;

       auto *exportButton = new QPushButton("Export Settings to Clipboard");
       exportButton->setToolTip("Copy current image uploader settings to clipboard as JSON");
       QObject::connect(exportButton, &QPushButton::clicked, [this]() {
           this->exportSettings();
       });
       buttonLayout->addWidget(exportButton);

       auto *importButton = new QPushButton("Import Settings from Clipboard");
       importButton->setToolTip("Import image uploader settings from clipboard JSON");
       QObject::connect(importButton, &QPushButton::clicked, [this]() {
           this->importSettings();
       });
       buttonLayout->addWidget(importButton);

       buttonLayout->addStretch();
       layout.addLayout(buttonLayout);
   }

   layout.addStretch();
}

void ExternalToolsPage::exportSettings()
{
   auto &s = *getSettings();

   QJsonObject settingsObj;
   settingsObj["Version"] = "1.0.0";
   settingsObj["Name"] = "Chatterino Image Uploader Settings";
   settingsObj["RequestMethod"] = "POST";
   settingsObj["RequestURL"] = s.imageUploaderUrl.getValue();
   settingsObj["Body"] = "MultipartFormData";
   settingsObj["FileFormName"] = s.imageUploaderFormField.getValue();
   settingsObj["URL"] = s.imageUploaderLink.getValue();
   settingsObj["DeletionURL"] = s.imageUploaderDeletionLink.getValue();

   QString headers = s.imageUploaderHeaders.getValue();
   if (!headers.isEmpty()) {
       QJsonObject headersObj;
       QStringList headerLines = headers.split('\n', Qt::SkipEmptyParts);
       for (const QString &line : headerLines) {
           QStringList parts = line.split(':', Qt::SkipEmptyParts);
           if (parts.size() >= 2) {
               QString key = parts[0].trimmed();
               QString value = parts.mid(1).join(':').trimmed();
               headersObj[key] = value;
           }
       }
       if (!headersObj.isEmpty()) {
           settingsObj["Headers"] = headersObj;
       }
   }

   QJsonDocument doc(settingsObj);
   QApplication::clipboard()->setText(doc.toJson(QJsonDocument::Indented));

   QMessageBox::information(this, "Settings Exported",
       "Image uploader settings have been copied to clipboard as JSON.");
}

void ExternalToolsPage::importSettings()
{
   QString clipboardText = QApplication::clipboard()->text().trimmed();
   QJsonObject settingsObj;

   if (!this->validateImportJson(clipboardText, settingsObj)) {
       return;
   }

   int ret = QMessageBox::question(this, "Import Settings",
       "This will overwrite your current image uploader settings. Continue?",
       QMessageBox::Yes | QMessageBox::No);

   if (ret != QMessageBox::Yes) {
       return;
   }

   if (this->applyImportedSettings(settingsObj)) {
       QMessageBox::information(this, "Import Successful",
           "Image uploader settings have been imported successfully!");
   } else {
       QMessageBox::warning(this, "Import Failed",
           "No valid image uploader settings found in the JSON.");
   }
}

bool ExternalToolsPage::validateImportJson(const QString &clipboardText, QJsonObject &settingsObj)
{
   if (clipboardText.isEmpty()) {
       QMessageBox::warning(this, "Import Failed",
           "Clipboard is empty. Please copy JSON settings to clipboard first.");
       return false;
   }

   QJsonParseError parseError;
   QJsonDocument doc = QJsonDocument::fromJson(clipboardText.toUtf8(), &parseError);

   if (parseError.error != QJsonParseError::NoError) {
       QMessageBox::warning(this, "Import Failed",
           QString("Invalid JSON format: %1").arg(parseError.errorString()));
       return false;
   }

   if (!doc.isObject()) {
       QMessageBox::warning(this, "Import Failed",
           "JSON must be an object containing settings.");
       return false;
   }

   settingsObj = doc.object();
   return true;
}

bool ExternalToolsPage::applyImportedSettings(const QJsonObject &settingsObj)
{
   auto &s = *getSettings();
   bool hasValidSettings = false;

   if (settingsObj.contains("RequestURL") && settingsObj["RequestURL"].isString()) {
       s.imageUploaderUrl = settingsObj["RequestURL"].toString();
       hasValidSettings = true;
   }

   if (settingsObj.contains("FileFormName") && settingsObj["FileFormName"].isString()) {
       s.imageUploaderFormField = settingsObj["FileFormName"].toString();
       hasValidSettings = true;
   }

   if (settingsObj.contains("URL") && settingsObj["URL"].isString()) {
       s.imageUploaderLink = settingsObj["URL"].toString();
       hasValidSettings = true;
   }

   if (settingsObj.contains("DeletionURL") && settingsObj["DeletionURL"].isString()) {
       s.imageUploaderDeletionLink = settingsObj["DeletionURL"].toString();
       hasValidSettings = true;
   }

   if (settingsObj.contains("Headers") && settingsObj["Headers"].isObject()) {
       this->parseAndApplyHeaders(settingsObj["Headers"].toObject());
       hasValidSettings = true;
   }

   if (hasValidSettings) {
       s.imageUploaderEnabled = true;
   }

   return hasValidSettings;
}

void ExternalToolsPage::parseAndApplyHeaders(const QJsonObject &headersObj)
{
   auto &s = *getSettings();
   QStringList headerLines;

   for (auto it = headersObj.begin(); it != headersObj.end(); ++it) {
       if (it.value().isString()) {
           headerLines.append(QString("%1: %2").arg(it.key(), it.value().toString()));
       }
   }

   if (!headerLines.isEmpty()) {
       s.imageUploaderHeaders = headerLines.join('\n');
   }
}

}  // namespace chatterino
