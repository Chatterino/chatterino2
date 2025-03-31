#include "widgets/dialogs/UpdateDialog.hpp"

#include "Application.hpp"
#include "singletons/Updates.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/Label.hpp"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino {

UpdateDialog::UpdateDialog()
    : BaseWindow({BaseWindow::Frameless, BaseWindow::TopMost,
                  BaseWindow::EnableCustomFrame, BaseWindow::DisableLayoutSave})
{
    auto layout =
        LayoutCreator<UpdateDialog>(this).setLayoutType<QVBoxLayout>();

    layout.emplace<Label>("You shouldn't be seeing this dialog.")
        .assign(&this->ui_.label)
        ->setWordWrap(true);

    auto buttons = layout.emplace<QDialogButtonBox>();
    auto *install = buttons->addButton("Install", QDialogButtonBox::AcceptRole);
    this->ui_.installButton = install;
    auto *dismiss = buttons->addButton("Dismiss", QDialogButtonBox::RejectRole);

    QObject::connect(install, &QPushButton::clicked, this, [this] {
        getApp()->getUpdates().installUpdates();
        this->close();
    });
    QObject::connect(dismiss, &QPushButton::clicked, this, [this] {
        this->buttonClicked.invoke(Dismiss);
        this->close();
    });

    this->updateStatusChanged(getApp()->getUpdates().getStatus());
    this->connections_.managedConnect(getApp()->getUpdates().statusUpdated,
                                      [this](auto status) {
                                          this->updateStatusChanged(status);
                                      });

    this->setScaleIndependantHeight(150);
    this->setScaleIndependantWidth(250);
}

void UpdateDialog::updateStatusChanged(Updates::Status status)
{
    this->ui_.installButton->setVisible(status == Updates::UpdateAvailable);

    switch (status)
    {
        case Updates::UpdateAvailable: {
            this->ui_.label->setText(
                (getApp()->getUpdates().isDowngrade()
                     ? QString(
                           "The version online (%1) seems to be lower than the "
                           "current (%2).\nEither a version was reverted or "
                           "you are running a newer build.\n\nDo you want to "
                           "download and install it?")
                           .arg(getApp()->getUpdates().getOnlineVersion(),
                                getApp()->getUpdates().getCurrentVersion())
                     : QString("An update (%1) is available.\n\nDo you want to "
                               "download and install it?")
                           .arg(getApp()->getUpdates().getOnlineVersion())));
            this->updateGeometry();
        }
        break;

        case Updates::SearchFailed: {
            this->ui_.label->setText("Failed to load version information.");
        }
        break;

        case Updates::Downloading: {
            this->ui_.label->setText(
                "Downloading updates.\n\nChatterino will restart "
                "automatically when the download is done.");
        }
        break;

        case Updates::DownloadFailed: {
            this->ui_.label->setText("Failed to download the update.");
        }
        break;

        case Updates::WriteFileFailed: {
            this->ui_.label->setText("Failed to save the update to disk.");
        }
        break;

        default:;
    }
}

}  // namespace chatterino
