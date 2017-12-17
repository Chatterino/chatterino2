#include "qualitypopup.hpp"

#include <QProcess>

namespace chatterino {
namespace widgets {

QualityPopup::QualityPopup(const QString &channel, const QString &path, QStringList options)
    : BaseWidget()
    , channel(channel)
    , path(path)
{
    this->initAsWindow();

    this->ui.okButton.setText("OK");
    this->ui.cancelButton.setText("Cancel");

    QObject::connect(&this->ui.okButton, &QPushButton::clicked, this,
                     &QualityPopup::okButtonClicked);
    QObject::connect(&this->ui.cancelButton, &QPushButton::clicked, this,
                     &QualityPopup::cancelButtonClicked);

    this->ui.buttonBox.addButton(&this->ui.okButton, QDialogButtonBox::ButtonRole::AcceptRole);
    this->ui.buttonBox.addButton(&this->ui.cancelButton, QDialogButtonBox::ButtonRole::RejectRole);

    for (int i = 0; i < options.length(); ++i) {
        this->ui.selector.addItem(options.at(i));
    }

    this->ui.vbox.addWidget(&this->ui.selector);
    this->ui.vbox.addWidget(&this->ui.buttonBox);

    this->setLayout(&this->ui.vbox);
}

void QualityPopup::showDialog(const QString &channel, const QString &path, QStringList options)
{
    static QualityPopup *instance = new QualityPopup(channel, path, options);

    instance->show();
    instance->activateWindow();
    instance->raise();
    instance->setFocus();
}

void QualityPopup::okButtonClicked()
{
    QProcess::startDetached(this->path,
                            {"twitch.tv/" + this->channel, this->ui.selector.currentText()});
    this->close();
}

void QualityPopup::cancelButtonClicked()
{
    this->close();
}

}  // namespace widgets
}  // namespace chatterino
