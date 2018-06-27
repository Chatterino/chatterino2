#include "LastRunCrashDialog.hpp"

#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "singletons/UpdateManager.hpp"
#include "util/LayoutCreator.hpp"
#include "util/PostToThread.hpp"

namespace chatterino {

LastRunCrashDialog::LastRunCrashDialog()
{
    this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    this->setWindowTitle("Chatterino");

    auto &updateManager = chatterino::UpdateManager::getInstance();

    auto layout = LayoutCreator<LastRunCrashDialog>(this).setLayoutType<QVBoxLayout>();

    layout.emplace<QLabel>(
        "The application wasn't terminated properly the last time it was executed.");

    layout->addSpacing(16);

    //    auto update = layout.emplace<QLabel>();
    auto buttons = layout.emplace<QDialogButtonBox>();

    //    auto *installUpdateButton = buttons->addButton("Install Update",
    //    QDialogButtonBox::NoRole); installUpdateButton->setEnabled(false);
    //    QObject::connect(installUpdateButton, &QPushButton::clicked, [this, update]() mutable {
    //        auto &updateManager = chatterino::UpdateManager::getInstance();

    //        updateManager.installUpdates();
    //        this->setEnabled(false);
    //        update->setText("Downloading updates...");
    //    });

    auto *okButton = buttons->addButton("Ignore", QDialogButtonBox::ButtonRole::NoRole);
    QObject::connect(okButton, &QPushButton::clicked, [this] { this->accept(); });

    // Updates
    //    auto updateUpdateLabel = [update]() mutable {
    //        auto &updateManager = chatterino::UpdateManager::getInstance();

    //        switch (updateManager.getStatus()) {
    //            case chatterino::UpdateManager::None: {
    //                update->setText("Not checking for updates.");
    //            } break;
    //            case chatterino::UpdateManager::Searching: {
    //                update->setText("Checking for updates...");
    //            } break;
    //            case chatterino::UpdateManager::UpdateAvailable: {
    //                update->setText("Update available.");
    //            } break;
    //            case chatterino::UpdateManager::NoUpdateAvailable: {
    //                update->setText("No update abailable.");
    //            } break;
    //            case chatterino::UpdateManager::SearchFailed: {
    //                update->setText("Error while searching for update.\nEither the update service
    //                is "
    //                                "temporarily down or there is an issue with your
    //                                installation.");
    //            } break;
    //            case chatterino::UpdateManager::Downloading: {
    //                update->setText(
    //                    "Downloading the update. Chatterino will close once the download is
    //                    done.");
    //            } break;
    //            case chatterino::UpdateManager::DownloadFailed: {
    //                update->setText("Download failed.");
    //            } break;
    //            case chatterino::UpdateManager::WriteFileFailed: {
    //                update->setText("Writing the update file to the hard drive failed.");
    //            } break;
    //        }
    //    };

    //    updateUpdateLabel();
    //    this->managedConnect(updateManager.statusUpdated, [updateUpdateLabel](auto) mutable {
    //        postToThread([updateUpdateLabel]() mutable { updateUpdateLabel(); });
    //    });
}

}  // namespace chatterino
