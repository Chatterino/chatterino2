#include "StreamSettingsDialog.hpp"

#include "Application.hpp"
#include "common/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/WindowManager.hpp"
#include "ui_StreamSettingsDialog.h"
#include "util/PostToThread.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/Window.hpp"

#include <QMessageBox>

#include <chrono>

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
    this->ui_->tagPicker->lineEdit()->setPlaceholderText("Pick tags here");
}

StreamSettingsDialog::~StreamSettingsDialog()
{
    delete this->ui_;
}

void StreamSettingsDialog::updateGameSearch()
{
    this->lastUpdateTimer_.start(1s);
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

    if (gameId.isEmpty() && title.isEmpty() && !this->needUpdateTags)
    {
        QMessageBox::warning(
            this, "No changes to submit.",
            "There were no changes to submit in the stream settings");
        return;
    }

    if (!gameId.isEmpty() || !title.isEmpty())
    {
        getHelix()->updateChannel(
            this->roomId_, gameId, "", title,
            [](NetworkResult result) {
                qCDebug(chatterinoCommon) << "OK!" << result.status();
            },
            [] {
                QMessageBox::warning(
                    getApp()->windows->getMainWindow().window(),
                    "Failed to submit changes.",
                    "General: API responded with an error.");
            });
    }

    if (this->needUpdateTags)
    {
        qCDebug(chatterinoCommon) << "Updating tags...";
        auto tags = QStringList();
        for (int i = 0; auto ptr = this->ui_->selectedTagsList->item(i); i++)
        {
            auto tagId = ptr->data(Qt::UserRole).toString();
            qCDebug(chatterinoCommon) << ptr->text() << tagId;

            if (!tagId.isEmpty())
            {
                tags.append(tagId);
            }
        }
        getHelix()->updateStreamTags(
            this->roomId_, tags,
            [] {
                qCDebug(chatterinoCommon) << "Update tags: OK";
            },
            [] {
                qCDebug(chatterinoCommon) << "Update tags: fail";
                QMessageBox::warning(
                    getApp()->windows->getMainWindow().window(),
                    "Failed to submit changes.",
                    "Tags: API responded with an error.");
            });
    }
}

void StreamSettingsDialog::queueLoadTags()
{
    if (this->ui_->tabWidget->currentIndex() != 1 || this->isLoadingTags ||
        this->cachedTags_.size() != 0)
    {
        return;
    }
    qCDebug(chatterinoCommon) << "queueLoadTags";
    this->isLoadingTags = true;
    reallyLoadTags();
}

void StreamSettingsDialog::addTag()
{
    auto text = this->ui_->tagPicker->currentText();
    auto item = new QListWidgetItem(text, this->ui_->selectedTagsList);
    item->setData(Qt::UserRole, this->ui_->tagPicker->currentData());
    this->ui_->selectedTagsList->addItem(item);
    this->needUpdateTags = true;
}

void StreamSettingsDialog::removeTag()
{
    qDeleteAll(this->ui_->selectedTagsList->selectedItems());
}

void StreamSettingsDialog::reallyLoadTags()
{
    getHelix()->getStreamTags(
        this->roomId_,
        [this](std::vector<HelixTag> tags) {
            for (auto tag : tags)
            {
                if (tag.isAuto)
                {
                    this->ui_->selectedTagsList->addItem(
                        QString("%1 (auto)").arg(tag.englishName));
                }
                else
                {
                    auto item = new QListWidgetItem(
                        tag.englishName, this->ui_->selectedTagsList);
                    item->setData(Qt::UserRole, tag.id);
                    this->ui_->selectedTagsList->addItem(item);
                }
            }
        },
        [this] {
            QMessageBox::warning(this, "Failed to look up tags for channel.",
                                 "Something failed.");
        });
    QTimer::singleShot(500ms, this, &StreamSettingsDialog::reallyLoadAllTags);
}

void StreamSettingsDialog::reallyLoadAllTags()
{
    getHelix()->fetchStreamTags(
        this->lastTagsCursor_,
        [this](std::vector<HelixTag> tags, QString cursor) {
            qCDebug(chatterinoCommon)
                << "Cursor:" << cursor << " tags: " << tags.size();
            this->lastTagsCursor_ = cursor;
            this->cachedTags_.insert(this->cachedTags_.end(), tags.begin(),
                                     tags.end());
            this->isLoadingTags = tags.size() != 0 && !cursor.isEmpty();
            for (auto tag : tags)
            {
                if (!tag.isAuto)
                {
                    this->ui_->tagPicker->addItem(tag.englishName, tag.id);
                }
            }
            this->ui_->tagPicker->model()->sort(0);

            if (this->isLoadingTags)
            {
                qCDebug(chatterinoCommon) << "Setting up timer;";
                QTimer::singleShot(500ms, this,
                                   &StreamSettingsDialog::reallyLoadAllTags);
            }
            else
            {
                this->ui_->tagLoadBar->hide();
            }
        },
        [this] {
            QMessageBox::warning(this, "Failed to look up tags.",
                                 "Something failed.");
            this->isLoadingTags = false;
            this->ui_->tagLoadBar->hide();
        });
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
}  // namespace chatterino
