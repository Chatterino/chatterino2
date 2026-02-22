// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/ChatterListWidget.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"  // IWYU pragma: keep
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"
#include "util/Helpers.hpp"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

namespace chatterino {

namespace {

QString formatVIPListError(HelixListVIPsError error, const QString &message)
{
    using Error = HelixListVIPsError;

    QString errorMessage = QString("Failed to list VIPs - ");

    switch (error)
    {
        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::Ratelimited: {
            errorMessage += "You are being ratelimited by Twitch. Try "
                            "again in a few seconds.";
        }
        break;

        case Error::UserMissingScope: {
            // TODO(pajlada): Phrase MISSING_REQUIRED_SCOPE
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            // TODO(pajlada): Phrase MISSING_PERMISSION
            errorMessage += "You don't have permission to "
                            "perform that action.";
        }
        break;

        case Error::UserNotBroadcaster: {
            errorMessage +=
                "Due to Twitch restrictions, "
                "this command can only be used by the broadcaster. "
                "To see the list of VIPs you must use the Twitch website.";
        }
        break;

        case Error::Unknown: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

QString formatModsError(HelixGetModeratorsError error, const QString &message)
{
    using Error = HelixGetModeratorsError;

    QString errorMessage = QString("Failed to get moderators: ");

    switch (error)
    {
        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::UserMissingScope: {
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            errorMessage +=
                "Due to Twitch restrictions, "
                "this command can only be used by the broadcaster. "
                "To see the list of mods you must use the Twitch website.";
        }
        break;

        case Error::Unknown: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

QString formatChattersError(HelixGetChattersError error, const QString &message)
{
    using Error = HelixGetChattersError;

    QString errorMessage = QString("Failed to get chatters: ");

    switch (error)
    {
        case Error::Forwarded: {
            errorMessage += message;
        }
        break;

        case Error::UserMissingScope: {
            errorMessage += "Missing required scope. "
                            "Re-login with your "
                            "account and try again.";
        }
        break;

        case Error::UserNotAuthorized: {
            errorMessage +=
                "Due to Twitch restrictions, "
                "this command can only be used by moderators. "
                "To see the list of chatters you must use the Twitch website.";
        }
        break;

        case Error::Unknown: {
            errorMessage += "An unknown error has occurred.";
        }
        break;
    }
    return errorMessage;
}

}  // namespace

void ChatterListWidget::setupUi()
{
    this->setWindowTitle("Chatter List - " + this->twitchChannel_->getName());
    assert(this->twitchChannel_ != nullptr);

    this->setAttribute(Qt::WA_DeleteOnClose);

    this->dockVbox_ = new QVBoxLayout(this);
    this->searchBar_ = new QLineEdit(this);

    this->chattersList_ = new QListWidget(this);
    this->resultList_ = new QListWidget(this);

    this->loadingLabel_ = new QLabel("Loading...", this);
    this->searchBar_->setPlaceholderText("Search User...");

    auto listDoubleClick = [this](const QModelIndex &index) {
        const auto itemText = index.data().toString();

        if (!itemText.isEmpty())
        {
            this->userClicked(itemText);
        }
    };

    QObject::connect(this->chattersList_, &QListWidget::doubleClicked, this,
                     listDoubleClick);

    QObject::connect(this->resultList_, &QListWidget::doubleClicked, this,
                     listDoubleClick);

    HotkeyController::HotkeyMap actions{
        {"delete",
         [this](const std::vector<QString> &) -> QString {
             this->close();
             return "";
         }},
        {"accept", nullptr},
        {"reject", nullptr},
        {"scrollPage", nullptr},
        {"openTab", nullptr},
        {"search",
         [this](const std::vector<QString> &) -> QString {
             this->searchBar_->setFocus();
             this->searchBar_->selectAll();
             return "";
         }},
    };

    getApp()->getHotkeys()->shortcutsForCategory(HotkeyCategory::PopupWindow,
                                                 actions, this);

    this->dockVbox_->addWidget(this->searchBar_);
    this->dockVbox_->addWidget(this->loadingLabel_);
    this->dockVbox_->addWidget(this->chattersList_);
    this->dockVbox_->addWidget(this->resultList_);
    this->resultList_->hide();

    this->setStyleSheet(this->theme->splits.input.styleSheet);
    this->setLayout(this->dockVbox_);

    this->setMinimumWidth(300);
}

void ChatterListWidget::clearUi()
{
    std::cout << "clearing the connected users UI" << std::endl;
    this->chatterList_.clear();
    this->modChatters_.clear();
    this->vipChatters_.clear();
    this->chattersList_->clear();
}

void ChatterListWidget::refresh()
{
    auto formatListItemText = [](const QString &text) {
        auto *item = new QListWidgetItem();
        item->setText(text);
        item->setFont(
            getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1.0));
        return item;
    };
    auto addLabel = [=, this](const QString &label) {
        auto *formattedLabel = formatListItemText(label);
        formattedLabel->setFlags(Qt::NoItemFlags);
        formattedLabel->setForeground(this->theme->accent);
        this->chattersList_->addItem(formattedLabel);
    };
    auto addUserList = [=, this](const QStringList &users, QString label) {
        if (users.isEmpty())
        {
            return;
        }

        addLabel(QString("%1 (%2)").arg(label, localizeNumbers(users.size())));

        for (const auto &user : users)
        {
            this->chattersList_->addItem(formatListItemText(user));
        }
        this->chattersList_->addItem(new QListWidgetItem());
    };

    auto performListSearch = [=, this]() {
        auto query = this->searchBar_->text();
        if (query.isEmpty())
        {
            this->resultList_->hide();
            this->chattersList_->show();
            return;
        }

        auto results = this->chattersList_->findItems(query, Qt::MatchContains);
        this->chattersList_->hide();
        this->resultList_->clear();
        for (auto &item : results)
        {
            if (!item->text().contains("("))
            {
                this->resultList_->addItem(formatListItemText(item->text()));
            }
        }
        this->resultList_->show();
    };

    auto loadChatters = [=, this](auto modList, auto vipList,
                                  bool isBroadcaster) {
        getHelix()->getChatters(
            this->twitchChannel_->roomId(),
            getApp()->getAccounts()->twitch.getCurrent()->getUserId(), 50000,
            [=, this](const auto &chatters) {
                ChatterListWidget::clearUi();
                auto broadcaster = this->twitchChannel_->getName().toLower();

                bool addedBroadcaster = false;
                for (auto chatter : chatters.chatters)
                {
                    chatter = chatter.toLower();

                    if (!addedBroadcaster && chatter == broadcaster)
                    {
                        addedBroadcaster = true;
                        addLabel("Broadcaster");
                        this->chattersList_->addItem(broadcaster);
                        this->chattersList_->addItem(new QListWidgetItem());
                        continue;
                    }

                    if (modList.contains(chatter))
                    {
                        this->modChatters_.append(chatter);
                        continue;
                    }

                    if (vipList.contains(chatter))
                    {
                        this->vipChatters_.append(chatter);
                        continue;
                    }

                    this->chatterList_.append(chatter);
                }

                this->modChatters_.sort();
                this->vipChatters_.sort();
                this->chatterList_.sort();

                if (isBroadcaster)
                {
                    addUserList(this->modChatters_, QString("Moderators"));
                    addUserList(vipChatters_, QString("VIPs"));
                }
                else
                {
                    addLabel("Moderators");
                    this->chattersList_->addItem(
                        "Moderators cannot check who is a moderator");
                    this->chattersList_->addItem(new QListWidgetItem());

                    addLabel("VIPs");
                    this->chattersList_->addItem(
                        "Moderators cannot check who is a VIP");
                    this->chattersList_->addItem(new QListWidgetItem());
                }

                addUserList(this->chatterList_, QString("Chatters"));

                this->loadingLabel_->hide();
                performListSearch();
            },
            [=, this](auto error, const auto &message) {
                auto errorMessage = formatChattersError(error, message);
                this->chattersList_->addItem(formatListItemText(errorMessage));
            });
    };

    QObject::connect(this->searchBar_, &QLineEdit::textEdited, this,
                     performListSearch);

    // Only broadcaster can get vips, mods can get chatters

    if (this->twitchChannel_->isBroadcaster())
    {
        // Add moderators
        getHelix()->getModerators(
            this->twitchChannel_->roomId(), 1000,
            [=, this](const auto &mods) {
                QSet<QString> modList;
                for (const auto &mod : mods)
                {
                    modList.insert(mod.userName.toLower());
                }

                // Add vips
                getHelix()->getChannelVIPs(
                    this->twitchChannel_->roomId(),
                    [=](const auto &vips) {
                        QSet<QString> vipList;
                        for (const auto &vip : vips)
                        {
                            vipList.insert(vip.userName.toLower());
                        }

                        // Add chatters
                        loadChatters(modList, vipList, true);
                    },
                    [=, this](auto error, const auto &message) {
                        auto errorMessage = formatVIPListError(error, message);
                        this->chattersList_->addItem(
                            formatListItemText(errorMessage));
                    });
            },
            [=, this](auto error, const auto &message) {
                auto errorMessage = formatModsError(error, message);
                this->chattersList_->addItem(formatListItemText(errorMessage));
            });
    }
    else if (this->twitchChannel_->hasModRights())
    {
        QSet<QString> modList;
        QSet<QString> vipList;
        loadChatters(modList, vipList, false);
    }
    else
    {
        this->chattersList_->addItem(
            formatListItemText("Due to Twitch restrictions, this feature is "
                               "only \navailable for moderators."));
        this->chattersList_->addItem(
            formatListItemText("If you would like to see the Chatter list, you "
                               "must \nuse the Twitch website."));
        this->loadingLabel_->hide();
    }
}

ChatterListWidget::ChatterListWidget(const TwitchChannel *twitchChannel,
                                     QWidget *parent)
    : BaseWindow({}, parent)
    , twitchChannel_(twitchChannel)
{
    this->setupUi();
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ChatterListWidget::refresh);
    timer->start(2000);
    this->refresh();
}

}  // namespace chatterino
