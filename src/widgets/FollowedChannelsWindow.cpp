// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/FollowedChannelsWindow.hpp"

#include "Application.hpp"
#include "common/Common.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Settings.hpp"
#include "util/Clipboard.hpp"
#include "util/CustomPlayer.hpp"
#include "util/Helpers.hpp"
#include "util/IncognitoBrowser.hpp"
#include "util/StreamLink.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/Window.hpp"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPointer>
#include <QSet>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <chrono>
#include <vector>

namespace chatterino {

namespace {

constexpr auto CHANNEL_LOGIN_ROLE = Qt::UserRole;
constexpr auto SORT_ROLE = Qt::UserRole + 1;
constexpr auto CHANNEL_ID_ROLE = Qt::UserRole + 2;
constexpr auto LIVE_ROLE = Qt::UserRole + 3;

constexpr std::array SEARCH_COLUMNS{0, 3, 4};

qint64 uptimeSeconds(const QString &startedAt)
{
    const auto started = QDateTime::fromString(startedAt, Qt::ISODate);
    if (!started.isValid())
    {
        return -1;
    }

    return started.secsTo(QDateTime::currentDateTimeUtc());
}

QString formatUptime(const QString &startedAt)
{
    const auto seconds = uptimeSeconds(startedAt);
    if (seconds < 0)
    {
        return {};
    }

    const auto hours = seconds / 3600;
    const auto minutes = (seconds % 3600) / 60;
    return QStringLiteral("%1h %2m").arg(hours).arg(minutes);
}

}  // namespace

class FollowedChannelsFilterModel final : public QSortFilterProxyModel
{
public:
    explicit FollowedChannelsFilterModel(QObject *parent)
        : QSortFilterProxyModel(parent)
    {
    }

    void setQuery(const QString &query)
    {
        const auto trimmed = query.trimmed();
        if (this->query == trimmed)
        {
            return;
        }

        this->query = trimmed;
        this->invalidateFilter();
    }

    void setShowOffline(bool showOffline)
    {
        if (this->showOffline == showOffline)
        {
            return;
        }

        this->showOffline = showOffline;
        this->invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override
    {
        const auto first =
            this->sourceModel()->index(sourceRow, 0, sourceParent);
        if (!this->showOffline && !first.data(LIVE_ROLE).toBool())
        {
            return false;
        }

        if (this->query.isEmpty())
        {
            return true;
        }

        if (first.data(CHANNEL_LOGIN_ROLE)
                .toString()
                .contains(this->query, Qt::CaseInsensitive))
        {
            return true;
        }

        return std::ranges::any_of(SEARCH_COLUMNS, [&](int column) {
            return this->sourceModel()
                ->index(sourceRow, column, sourceParent)
                .data(Qt::DisplayRole)
                .toString()
                .contains(this->query, Qt::CaseInsensitive);
        });
    }

    bool lessThan(const QModelIndex &left,
                  const QModelIndex &right) const override
    {
        if (left.data(this->sortRole()) != right.data(this->sortRole()))
        {
            return QSortFilterProxyModel::lessThan(left, right);
        }

        const auto leftName = left.sibling(left.row(), 0).data().toString();
        const auto rightName = right.sibling(right.row(), 0).data().toString();
        const auto comparison =
            leftName.compare(rightName, Qt::CaseInsensitive);
        return this->sortOrder() == Qt::AscendingOrder ? comparison < 0
                                                       : comparison > 0;
    }

private:
    QString query;
    bool showOffline{true};
};

FollowedChannelsWindow::FollowedChannelsWindow(Window *parent)
    : BasePopup(
          {BaseWindow::EnableCustomFrame, BaseWindow::DisableLayoutSave,
           BaseWindow::ClearBuffersOnDpiChange, BaseWindow::BoundsCheckOnShow},
          parent)
    , ui{
          .search = new QLineEdit(this),
          .showOffline = new QCheckBox("Show offline channels", this),
          .status = new QLabel(this),
          .model = new QStandardItemModel(this),
          .proxyModel = new FollowedChannelsFilterModel(this),
          .list = new QTableView(this),
      }
    , refreshTimer(new QTimer(this))
    , followedChannelsRefreshTimer(new QTimer(this))
    , window(parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle("Followed Channels");
    this->resize(int(750 * this->scale()), int(450 * this->scale()));

    this->ui.search->setPlaceholderText("Search channels");
    this->ui.showOffline->setChecked(true);

    this->ui.model->setColumnCount(5);
    this->ui.model->setHorizontalHeaderLabels(
        {"Channel", "Viewers", "Uptime", "Category", "Title"});

    this->ui.proxyModel->setSourceModel(this->ui.model);
    this->ui.proxyModel->setSortRole(SORT_ROLE);
    this->ui.proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

    this->ui.list->setModel(this->ui.proxyModel);
    this->ui.list->setAlternatingRowColors(true);
    this->ui.list->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->ui.list->setSelectionMode(QAbstractItemView::SingleSelection);
    this->ui.list->setShowGrid(false);
    this->ui.list->setSortingEnabled(true);
    this->ui.list->sortByColumn(1, Qt::DescendingOrder);
    this->ui.list->setContextMenuPolicy(Qt::CustomContextMenu);
    this->ui.list->verticalHeader()->hide();
    this->ui.list->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    this->ui.list->horizontalHeader()->setSectionResizeMode(
        4, QHeaderView::Stretch);

    auto *layout = new QVBoxLayout(this->getLayoutContainer());
    layout->addWidget(this->ui.search);
    layout->addWidget(this->ui.showOffline);
    layout->addWidget(this->ui.status);
    layout->addWidget(this->ui.list);

    QObject::connect(this->ui.search, &QLineEdit::textChanged, this,
                     [this](const QString &query) {
                         this->ui.proxyModel->setQuery(query);
                         this->updateStatus();
                     });
    QObject::connect(this->ui.showOffline, &QCheckBox::toggled, this,
                     [this](bool showOffline) {
                         this->ui.proxyModel->setShowOffline(showOffline);
                         this->updateStatus();
                     });
    QObject::connect(this->ui.list, &QAbstractItemView::activated, this,
                     [this](const QModelIndex &index) {
                         this->openChannelInNewTab(index.sibling(index.row(), 0)
                                                       .data(CHANNEL_LOGIN_ROLE)
                                                       .toString());
                     });
    QObject::connect(this->ui.list, &QTableView::customContextMenuRequested,
                     this, &FollowedChannelsWindow::showContextMenu);

    QObject::connect(this->refreshTimer, &QTimer::timeout, this, [this] {
        const auto account = getApp()->getAccounts()->twitch.getCurrent();
        if (!account->isAnon())
        {
            this->loadFollowedStreams(account->getUserId());
        }
    });
    QObject::connect(
        this->followedChannelsRefreshTimer, &QTimer::timeout, this, [this] {
            const auto account = getApp()->getAccounts()->twitch.getCurrent();
            if (!account->isAnon())
            {
                this->loadFollowedChannels(account->getUserId());
            }
        });

    this->signalHolder_.managedConnect(
        getApp()->getAccounts()->twitch.currentUserChanged, [this] {
            this->load();
        });
    this->signalHolder_.managedConnect(getApp()->getHotkeys()->onItemsUpdated,
                                       [this] {
                                           this->clearShortcuts();
                                           this->addShortcuts();
                                       });

    this->refreshTimer->start(std::chrono::minutes(1));
    this->followedChannelsRefreshTimer->start(std::chrono::minutes(5));

    this->addShortcuts();
    this->load();
}

void FollowedChannelsWindow::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"delete",
         [this](const std::vector<QString> &) -> QString {
             this->close();
             return {};
         }},
        {"reject", nullptr},
        {"accept", nullptr},
        {"scrollPage", nullptr},
        {"openTab", nullptr},
        {"search",
         [this](const std::vector<QString> &) -> QString {
             this->ui.search->setFocus();
             this->ui.search->selectAll();
             return {};
         }},
    };
    this->shortcuts_ = getApp()->getHotkeys()->shortcutsForCategory(
        HotkeyCategory::PopupWindow, actions, this);
}

void FollowedChannelsWindow::themeChangedEvent()
{
    BasePopup::themeChangedEvent();
    this->updateList();
}

void FollowedChannelsWindow::load()
{
    this->followedChannelsToken = CancellationToken{};
    this->streamsToken = CancellationToken{};
    this->followedChannels.clear();
    this->streams.clear();
    this->followedChannelsError.clear();
    this->streamsError.clear();
    this->loadingFollowedChannels = false;
    this->loadingStreams = false;
    this->ui.model->removeRows(0, this->ui.model->rowCount());

    const auto account = getApp()->getAccounts()->twitch.getCurrent();
    if (account->isAnon())
    {
        this->ui.status->setText("Log in to Twitch to see followed channels.");
        this->ui.status->show();
        return;
    }

    this->ui.status->setText("Loading followed channels...");
    this->ui.status->show();
    this->loadFollowedChannels(account->getUserId());
    this->loadFollowedStreams(account->getUserId());
}

void FollowedChannelsWindow::loadFollowedChannels(const QString &userID)
{
    this->loadingFollowedChannels = true;
    this->followedChannelsError.clear();
    this->pendingFollowedChannels.clear();

    CancellationToken token(false);
    this->followedChannelsToken = token;
    const auto requestToken = token;
    QPointer self(this);

    getHelix()->getFollowedChannels(
        userID,
        [this](const auto &channels, const auto &state) {
            for (const auto &channel : channels)
            {
                this->pendingFollowedChannels.insert_or_assign(
                    channel.broadcasterID, channel);
            }

            if (state.done)
            {
                this->loadingFollowedChannels = false;
                this->followedChannels.swap(this->pendingFollowedChannels);
                this->updateList();
            }
        },
        [self, requestToken](const QString &error) {
            if (self == nullptr || requestToken.isCancelled())
            {
                return;
            }

            self->loadingFollowedChannels = false;
            self->followedChannelsError = error;
            self->updateList();
        },
        std::move(token));
}

void FollowedChannelsWindow::loadFollowedStreams(const QString &userID)
{
    this->loadingStreams = true;
    this->streamsError.clear();
    this->pendingStreams.clear();

    CancellationToken token(false);
    this->streamsToken = token;
    const auto requestToken = token;
    QPointer self(this);

    getHelix()->getFollowedStreams(
        userID,
        [this](const auto &streams, const auto &state) {
            for (const auto &stream : streams)
            {
                this->pendingStreams.insert_or_assign(stream.userId, stream);
            }

            if (state.done)
            {
                this->loadingStreams = false;
                this->streams.swap(this->pendingStreams);
                this->updateList();
            }
        },
        [self, requestToken](const QString &error) {
            if (self == nullptr || requestToken.isCancelled())
            {
                return;
            }

            self->loadingStreams = false;
            self->streamsError = error;
            self->updateList();
        },
        std::move(token));
}

void FollowedChannelsWindow::updateList()
{
    struct Entry {
        QString id;
        QString login;
        QString name;
        const HelixStream *stream{};
    };

    std::vector<Entry> entries;
    entries.reserve(this->streams.size() + this->followedChannels.size());
    for (const auto &[id, stream] : this->streams)
    {
        entries.push_back({id, stream.userLogin, stream.userName, &stream});
    }

    for (const auto &[id, channel] : this->followedChannels)
    {
        if (!this->streams.contains(id))
        {
            entries.push_back({id, channel.broadcasterLogin,
                               channel.broadcasterName, nullptr});
        }
    }

    QHash<QString, QStandardItem *> existingRows;
    for (int row = 0; row < this->ui.model->rowCount(); ++row)
    {
        auto *item = this->ui.model->item(row, 0);
        existingRows.insert(item->data(CHANNEL_ID_ROLE).toString(), item);
    }

    QSet<QString> currentIDs;
    QList<QStandardItem *> items;
    items.reserve(this->ui.model->columnCount());

    for (const auto &entry : entries)
    {
        currentIDs.insert(entry.id);
        const auto *stream = entry.stream;

        QStringList columns{entry.name, {}, {}, {}, {}};
        QList<QVariant> sortValues{entry.name.toCaseFolded(), -1, -1, {}, {}};
        if (stream != nullptr)
        {
            columns[1] = localizeNumbers(stream->viewerCount);
            columns[2] = formatUptime(stream->startedAt);
            columns[3] = stream->gameName;
            columns[4] = stream->title;

            sortValues[1] = stream->viewerCount;
            sortValues[2] = uptimeSeconds(stream->startedAt);
            sortValues[3] = stream->gameName.toCaseFolded();
            sortValues[4] = stream->title.toCaseFolded();
        }

        const auto existing = existingRows.find(entry.id);
        if (existing == existingRows.end())
        {
            items.clear();
            for (int column = 0; column < columns.size(); ++column)
            {
                auto *item = new QStandardItem;
                item->setEditable(false);
                items.append(item);
            }

            this->ui.model->appendRow(items);
        }

        const auto row = existing == existingRows.end()
                             ? this->ui.model->rowCount() - 1
                             : existing.value()->row();
        for (int column = 0; column < columns.size(); ++column)
        {
            auto *item = this->ui.model->item(row, column);
            item->setText(columns[column]);
            item->setData(sortValues[column], SORT_ROLE);
            if (stream == nullptr)
            {
                item->setForeground(
                    this->palette().color(QPalette::Disabled, QPalette::Text));
            }
            else
            {
                item->setData(QVariant{}, Qt::ForegroundRole);
            }
        }

        auto *first = this->ui.model->item(row, 0);
        first->setData(entry.id, CHANNEL_ID_ROLE);
        first->setData(entry.login, CHANNEL_LOGIN_ROLE);
        first->setData(stream != nullptr, LIVE_ROLE);
    }

    for (int row = this->ui.model->rowCount() - 1; row >= 0; --row)
    {
        if (!currentIDs.contains(
                this->ui.model->item(row, 0)->data(CHANNEL_ID_ROLE).toString()))
        {
            this->ui.model->removeRow(row);
        }
    }

    this->updateStatus();
}

void FollowedChannelsWindow::updateStatus()
{
    const auto query = this->ui.search->text().trimmed();
    const auto account = getApp()->getAccounts()->twitch.getCurrent();

    if (account->isAnon())
    {
        this->ui.status->setText("Log in to Twitch to see followed channels.");
    }
    else if (!this->streamsError.isEmpty())
    {
        this->ui.status->setText("Failed to load followed streams: " +
                                 this->streamsError);
    }
    else if (this->ui.showOffline->isChecked() &&
             !this->followedChannelsError.isEmpty())
    {
        this->ui.status->setText("Failed to load followed channels: " +
                                 this->followedChannelsError);
    }
    else if (this->loadingStreams || (this->ui.showOffline->isChecked() &&
                                      this->loadingFollowedChannels))
    {
        this->ui.status->setText("Loading followed channels...");
    }
    else if (this->ui.proxyModel->rowCount() == 0)
    {
        if (!query.isEmpty())
        {
            this->ui.status->setText("No matching channels.");
        }
        else if (this->ui.showOffline->isChecked())
        {
            this->ui.status->setText("No followed channels found.");
        }
        else
        {
            this->ui.status->setText("No followed channels are live.");
        }
    }
    else
    {
        this->ui.status->clear();
    }

    this->ui.status->setVisible(!this->ui.status->text().isEmpty());
}

void FollowedChannelsWindow::openChannelInNewTab(const QString &channelLogin)
{
    if (channelLogin.isEmpty())
    {
        return;
    }

    auto *tab = this->window->getNotebook().addPage(true);
    tab->appendNewSplit(false)->setChannel(
        getApp()->getTwitch()->getOrAddChannel(channelLogin));
}

void FollowedChannelsWindow::openChannelInNewSplit(const QString &channelLogin)
{
    if (channelLogin.isEmpty())
    {
        return;
    }

    this->window->getNotebook()
        .getOrAddSelectedPage()
        ->appendNewSplit(false)
        ->setChannel(getApp()->getTwitch()->getOrAddChannel(channelLogin));
}

QString FollowedChannelsWindow::selectedChannel() const
{
    const auto index = this->ui.list->currentIndex();
    if (!index.isValid())
    {
        return {};
    }
    return index.sibling(index.row(), 0).data(CHANNEL_LOGIN_ROLE).toString();
}

void FollowedChannelsWindow::showContextMenu(QPoint position)
{
    this->ui.list->setCurrentIndex(this->ui.list->indexAt(position));
    const auto channel = this->selectedChannel();
    if (channel.isEmpty())
    {
        return;
    }

    const auto url = "https://twitch.tv/" + channel;
    auto *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    menu->addAction("&Open link", this, [url] {
        QDesktopServices::openUrl(QUrl(url));
    });
    if (supportsIncognitoLinks())
    {
        menu->addAction("Open link &incognito", this, [url] {
            openLinkIncognito(url);
        });
    }
    menu->addAction("&Copy link", this, [url] {
        crossPlatformCopy(url);
    });
    menu->addSeparator();

    menu->addAction("&Open in new split", this, [this, channel] {
        this->openChannelInNewSplit(channel);
    });
    menu->addAction("Open in new &tab", this, [this, channel] {
        this->openChannelInNewTab(channel);
    });
    menu->addSeparator();

    menu->addAction("Open player in &browser", this, [channel] {
        const auto playerUrl = TWITCH_PLAYER_URL.arg(channel);
        if (getSettings()->openLinksIncognito && supportsIncognitoLinks())
        {
            openLinkIncognito(playerUrl);
        }
        else
        {
            QDesktopServices::openUrl(QUrl(playerUrl));
        }
    });
    menu->addAction("Open in &streamlink", this, [channel] {
        openStreamlinkForChannelOrUrl(channel);
    });
    if (!getSettings()->customURIScheme.getValue().isEmpty())
    {
        menu->addAction("Open in custom &player", this, [channel] {
            openInCustomPlayer(channel);
        });
    }

    menu->popup(this->ui.list->viewport()->mapToGlobal(position));
}

}  // namespace chatterino
