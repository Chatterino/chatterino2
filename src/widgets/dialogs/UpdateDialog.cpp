// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/dialogs/UpdateDialog.hpp"

#include "Application.hpp"
#include "common/Version.hpp"
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
    this->windowDeactivateAction = WindowDeactivateAction::Delete;

    auto layout =
        LayoutCreator<UpdateDialog>(this).setLayoutType<QVBoxLayout>();

    layout.emplace<Label>("You shouldn't be seeing this dialog.")
        .assign(&this->ui_.label)
        ->setWordWrap(true);

    auto buttons = layout.emplace<QDialogButtonBox>();

    const auto *installText = [] {
        if (Version::instance().isNightly())
        {
            return "Yes";
        }

        return "Install";
    }();
    auto *install =
        buttons->addButton(installText, QDialogButtonBox::AcceptRole);
    this->ui_.installButton = install;
    auto *dismiss = buttons->addButton("Dismiss", QDialogButtonBox::RejectRole);

    QObject::connect(install, &QPushButton::clicked, this, [this] {
        getApp()->getUpdates().installUpdates();
        this->close();
    });
    QObject::connect(dismiss, &QPushButton::clicked, this, [this] {
        this->dismissed.invoke();
        this->close();
    });

    this->updateStatusChanged(getApp()->getUpdates().getStatus());
    this->connections_.managedConnect(getApp()->getUpdates().statusUpdated,
                                      [this](auto status) {
                                          this->updateStatusChanged(status);
                                      });

    this->setScaleIndependentHeight(150);
    this->setScaleIndependentWidth(250);
}

void UpdateDialog::updateStatusChanged(Updates::Status status)
{
    this->ui_.installButton->setVisible(status == Updates::UpdateAvailable);

    switch (status)
    {
        case Updates::UpdateAvailable: {
            this->ui_.label->setText(
                getApp()->getUpdates().buildUpdateAvailableText());
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

        case Updates::MissingPortableUpdater: {
            this->ui_.label->setText("The portable updater (expected in " %
                                     Updates::portableUpdaterPath() %
                                     ") was not found.");
        }
        break;

        case Updates::RunUpdaterFailed: {
            this->ui_.label->setText("Failed to run the updater.");
        }
        break;

        default:;
    }
}

}  // namespace chatterino
