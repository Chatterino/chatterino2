#include "widgets/dialogs/QualityPopup.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/WindowManager.hpp"
#include "util/StreamLink.hpp"
#include "widgets/Window.hpp"

namespace chatterino {

QualityPopup::QualityPopup(const QString &channelURL, QStringList options)
    : BasePopup(
          {
              BaseWindow::DisableLayoutSave,
              BaseWindow::BoundsCheckOnShow,
          },
          static_cast<QWidget *>(&(getApp()->getWindows()->getMainWindow())))
    , channelURL_(channelURL)
{
    this->ui_.selector = new QComboBox(this);
    this->ui_.vbox = new QVBoxLayout(this);
    this->ui_.buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    QObject::connect(this->ui_.buttonBox, &QDialogButtonBox::accepted, this,
                     &QualityPopup::okButtonClicked);
    QObject::connect(this->ui_.buttonBox, &QDialogButtonBox::rejected, this,
                     &QualityPopup::cancelButtonClicked);

    this->ui_.selector->addItems(options);

    this->ui_.vbox->addWidget(this->ui_.selector);
    this->ui_.vbox->addWidget(this->ui_.buttonBox);

    this->setLayout(this->ui_.vbox);
}

void QualityPopup::showDialog(const QString &channelURL, QStringList options)
{
    QualityPopup *instance = new QualityPopup(channelURL, options);

    instance->window()->setWindowTitle("Chatterino - select stream quality");
    instance->setAttribute(Qt::WA_DeleteOnClose, true);

    instance->show();
    instance->activateWindow();
    instance->raise();
}

void QualityPopup::keyPressEvent(QKeyEvent *e)
{
    if (this->handleEscape(e, this->ui_.buttonBox))
    {
        return;
    }
    if (this->handleEnter(e, this->ui_.buttonBox))
    {
        return;
    }

    BasePopup::keyPressEvent(e);
}

void QualityPopup::okButtonClicked()
{
    try
    {
        openStreamlink(this->channelURL_, this->ui_.selector->currentText());
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
