#include "QualityPopup.hpp"
#include "debug/Log.hpp"
#include "util/StreamLink.hpp"

namespace chatterino {
namespace widgets {

QualityPopup::QualityPopup(const QString &_channelName, QStringList options)
    : channelName(_channelName)
{
    this->ui_.okButton.setText("OK");
    this->ui_.cancelButton.setText("Cancel");

    QObject::connect(&this->ui_.okButton, &QPushButton::clicked, this,
                     &QualityPopup::okButtonClicked);
    QObject::connect(&this->ui_.cancelButton, &QPushButton::clicked, this,
                     &QualityPopup::cancelButtonClicked);

    this->ui_.buttonBox.addButton(&this->ui_.okButton, QDialogButtonBox::ButtonRole::AcceptRole);
    this->ui_.buttonBox.addButton(&this->ui_.cancelButton, QDialogButtonBox::ButtonRole::RejectRole);

    this->ui_.selector.addItems(options);

    this->ui_.vbox.addWidget(&this->ui_.selector);
    this->ui_.vbox.addWidget(&this->ui_.buttonBox);

    this->setLayout(&this->ui_.vbox);
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

    try {
        streamlink::OpenStreamlink(channelURL, this->ui_.selector.currentText());
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
