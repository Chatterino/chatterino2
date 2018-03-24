#include "qualitypopup.hpp"
#include "debug/log.hpp"
#include "util/streamlink.hpp"

namespace chatterino {
namespace widgets {

QualityPopup::QualityPopup(const QString &_channelName, QStringList options)
    : channelName(_channelName)
{
    this->ui.okButton.setText("OK");
    this->ui.cancelButton.setText("Cancel");

    QObject::connect(&this->ui.okButton, &QPushButton::clicked, this,
                     &QualityPopup::okButtonClicked);
    QObject::connect(&this->ui.cancelButton, &QPushButton::clicked, this,
                     &QualityPopup::cancelButtonClicked);

    this->ui.buttonBox.addButton(&this->ui.okButton, QDialogButtonBox::ButtonRole::AcceptRole);
    this->ui.buttonBox.addButton(&this->ui.cancelButton, QDialogButtonBox::ButtonRole::RejectRole);

    this->ui.selector.addItems(options);

    this->ui.vbox.addWidget(&this->ui.selector);
    this->ui.vbox.addWidget(&this->ui.buttonBox);

    this->setLayout(&this->ui.vbox);
}

void QualityPopup::showDialog(const QString &channelName, QStringList options)
{
    QualityPopup *instance = new QualityPopup(channelName, options);

    instance->setAttribute(Qt::WA_DeleteOnClose, true);

    instance->show();
    instance->activateWindow();
    instance->raise();
    instance->setFocus();
}

void QualityPopup::okButtonClicked()
{
    QString channelURL = "twitch.tv/" + this->channelName;

    singletons::SettingManager &settings = singletons::SettingManager::getInstance();

    try {
        streamlink::OpenStreamlink(channelURL, this->ui.selector.currentText());
    } catch (const streamlink::Exception &ex) {
        debug::Log("Exception caught trying to open streamlink: {}", ex.what());
    }

    this->close();
}

void QualityPopup::cancelButtonClicked()
{
    this->close();
}

}  // namespace widgets
}  // namespace chatterino
