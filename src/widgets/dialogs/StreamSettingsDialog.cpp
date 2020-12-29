#include "StreamSettingsDialog.hpp"
#include <chrono>
#include "Application.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/WindowManager.hpp"
#include "ui_StreamSettingsDialog.h"
#include "widgets/Window.hpp"

namespace chatterino {
using namespace std::chrono_literals;

StreamSettingsDialog::StreamSettingsDialog(const ChannelPtr channel)
    : ui_(new Ui::StreamSettingsDialog)
{
    ui_->setupUi(this);
    //this->channel_ = channel;

    this->window()->setWindowTitle(
        QString("%1 - Stream Settings").arg(channel->getName()));

    QObject::connect(&this->lastUpdateTimer_, &QTimer::timeout, this,
                     &StreamSettingsDialog::reallyUpdateGameSearch);
    QObject::connect(this->ui_->gameBox->lineEdit(), &QLineEdit::textEdited,
                     this, &StreamSettingsDialog::updateGameSearch);
    this->lastUpdateTimer_.setSingleShot(true);
    if (auto twitchChannel = dynamic_cast<TwitchChannel *>(channel.get()))
    {
        auto status = twitchChannel->accessStreamStatus();
        this->originalGame_ = status->game;
        this->originalTitle_ = status->title;

        this->ui_->gameBox->addItem(this->originalGame_);
        this->ui_->gameBox->setCurrentText(this->originalGame_);
        this->ui_->titleEdit->setText(this->originalTitle_);
        this->roomId_ = twitchChannel->roomId();
    }
}
StreamSettingsDialog::~StreamSettingsDialog()
{
    delete this->ui_;
}

void StreamSettingsDialog::updateGameSearch()
{
    this->lastUpdateTimer_.start(1s);
}

void StreamSettingsDialog::reallyUpdateGameSearch()
{
    qCDebug(chatterinoCommon)
        << "update game search :)" << this->ui_->gameBox->currentText();
    getHelix()->searchGames(
        this->ui_->gameBox->currentText(),
        [this](std::vector<HelixGame> games) {
            qCDebug(chatterinoCommon) << "Got" << games.size() << "games";
            auto searchText = this->ui_->gameBox->currentText();
            this->ui_->gameBox->clear();
            foreach (auto game, games)
            {
                this->ui_->gameBox->addItem(game.name, game.id);
            }
            // restore state
            this->ui_->gameBox->lineEdit()->setText(searchText);
        },
        [] {
            //
        });
}

void StreamSettingsDialog::accept()
{
    QString gameId;
    if (this->originalGame_ != this->ui_->gameBox->currentText())
    {
        QString gameName = this->ui_->gameBox->currentText();
        gameId = this->ui_->gameBox->currentData().toString();
        if (gameId.isNull() || gameId.isEmpty())
        {
            QMessageBox::warning(this, "Failed to look up games.",
                                 "Game not found.");
        }
        return;
    }

    QString title;
    if (this->originalTitle_ != this->ui_->titleEdit->text())
    {
        title = this->ui_->titleEdit->text();
    }

    if (gameId.isEmpty() && title.isEmpty())
    {
        QMessageBox::warning(
            this, "No changes to submit.",
            "There were no changes to submit in the stream settings");
    }

    getHelix()->updateChannel(
        this->roomId_, gameId, "", title,
        [](NetworkResult result) {
            qCDebug(chatterinoCommon) << "OK!" << result.status();
        },
        [] {
            QMessageBox::warning(getApp()->windows->getMainWindow().window(),
                                 "Failed to submit changes.",
                                 "API responded with an error.");
        });
}

}  // namespace chatterino
