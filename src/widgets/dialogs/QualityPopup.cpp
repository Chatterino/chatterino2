#include "QualityPopup.hpp"
#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/WindowManager.hpp"
#include "util/StreamLink.hpp"
#include "widgets/Window.hpp"

namespace chatterino {

QualityPopup::QualityPopup(const QString &_channelName, QStringList options,
                           QStringList extraArguments, bool streamMPV)
    : BasePopup({},
                static_cast<QWidget *>(&(getApp()->windows->getMainWindow())))
    , channelName_(_channelName)
    , extraArguments_(extraArguments)
    , streamMPV_(streamMPV)
{
    this->ui_.okButton.setText("OK");
    this->ui_.cancelButton.setText("Cancel");

    QObject::connect(&this->ui_.okButton, &QPushButton::clicked, this,
                     &QualityPopup::okButtonClicked);
    QObject::connect(&this->ui_.cancelButton, &QPushButton::clicked, this,
                     &QualityPopup::cancelButtonClicked);

    this->ui_.buttonBox.addButton(&this->ui_.okButton,
                                  QDialogButtonBox::ButtonRole::AcceptRole);
    this->ui_.buttonBox.addButton(&this->ui_.cancelButton,
                                  QDialogButtonBox::ButtonRole::RejectRole);

    this->ui_.selector.addItems(options);

    this->ui_.vbox.addWidget(&this->ui_.selector);
    this->ui_.vbox.addWidget(&this->ui_.buttonBox);

    this->setLayout(&this->ui_.vbox);
}

void QualityPopup::showDialog(const QString &channelName, QStringList options,
                              QStringList extraArguments, bool streamMPV)
{
    QualityPopup *instance =
        new QualityPopup(channelName, options, extraArguments, streamMPV);

    instance->window()->setWindowTitle("Chatterino - select stream quality");
    instance->setAttribute(Qt::WA_DeleteOnClose, true);

    instance->show();
    instance->activateWindow();
    instance->raise();
    instance->setFocus();
}

void QualityPopup::okButtonClicked()
{
    QString channelURL = "twitch.tv/" + this->channelName_;

    try
    {
        openStreamlink(channelURL, this->ui_.selector.currentText(),
                       this->extraArguments_, this->streamMPV_);
    }
    catch (const Exception &ex)
    {
        qCWarning(chatterinoWidget)
            << "Exception caught trying to open streamlink:" << ex.what();
    }

    this->close();
}

void QualityPopup::cancelButtonClicked()
{
    this->close();
}

}  // namespace chatterino
