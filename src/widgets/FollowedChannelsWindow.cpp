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
        if (this->query_ == trimmed)
        {
            return;
        }
        this->query_ = trimmed;
        this->invalidateFilter();
    }

    void setShowOffline(bool showOffline)
    {
        if (this->showOffline_ == showOffline)
        {
            return;
        }
        this->showOffline_ = showOffline;
        this->invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override
    {
        const auto first =
            this->sourceModel()->index(sourceRow, 0, sourceParent);
        if (!this->showOffline_ && !first.data(LIVE_ROLE).toBool())
        {
            return false;
        }
        if (this->query_.isEmpty())
        {
            return true;
        }
        if (first.data(CHANNEL_LOGIN_ROLE)
                .toString()
                .contains(this->query_, Qt::CaseInsensitive))
        {
            return true;
        }
        return std::ranges::any_of(SEARCH_COLUMNS, [&](int column) {
            return this->sourceModel()
                ->index(sourceRow, column, sourceParent)
                .data(Qt::DisplayRole)
                .toString()
                .contains(this->query_, Qt::CaseInsensitive);
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
    QString query_;
    bool showOffline_{true};
};

FollowedChannelsWindow::FollowedChannelsWindow(Window *parent)
    : BasePopup(
          {BaseWindow::EnableCustomFrame, BaseWindow::DisableLayoutSave,
           BaseWindow::ClearBuffersOnDpiChange, BaseWindow::BoundsCheckOnShow},
          parent)
    , search_(new QLineEdit(this))
    , showOffline_(new QCheckBox("Show offline channels", this))
    , status_(new QLabel(this))
    , model_(new QStandardItemModel(this))
    , proxyModel_(new FollowedChannelsFilterModel(this))
    , list_(new QTableView(this))
    , refreshTimer_(new QTimer(this))
    , followedChannelsRefreshTimer_(new QTimer(this))
    , window_(parent)
{
    this->setAttribute(Qt::WA_DeleteOnClose);
    this->setWindowTitle("Followed Channels");
    this->resize(int(750 * this->scale()), int(450 * this->scale()));

    this->search_->setPlaceholderText("Search channels");
    this->showOffline_->setChecked(true);
    this->model_->setColumnCount(5);
    this->model_->setHorizontalHeaderLabels(
        {"Channel", "Viewers", "Uptime", "Category", "Title"});
    this->proxyModel_->setSourceModel(this->model_);
    this->proxyModel_->setSortRole(SORT_ROLE);
    this->proxyModel_->setSortCaseSensitivity(Qt::CaseInsensitive);
    this->list_->setModel(this->proxyModel_);
    this->list_->setAlternatingRowColors(true);
    this->list_->setSelectionBehavior(QAbstractItemView::SelectRows);
    this->list_->setSelectionMode(QAbstractItemView::SingleSelection);
    this->list_->setShowGrid(false);
    this->list_->setSortingEnabled(true);
    this->list_->sortByColumn(1, Qt::DescendingOrder);
    this->list_->setContextMenuPolicy(Qt::CustomContextMenu);
    this->list_->verticalHeader()->hide();
    this->list_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    this->list_->horizontalHeader()->setSectionResizeMode(4,
                                                          QHeaderView::Stretch);

    auto *layout = new QVBoxLayout(this->getLayoutContainer());
    layout->addWidget(this->search_);
    layout->addWidget(this->showOffline_);
    layout->addWidget(this->status_);
    layout->addWidget(this->list_);

    QObject::connect(this->search_, &QLineEdit::textChanged, this,
                     [this](const QString &query) {
                         this->proxyModel_->setQuery(query);
                         this->updateStatus();
                     });
    QObject::connect(this->showOffline_, &QCheckBox::toggled, this,
                     [this](bool showOffline) {
                         this->proxyModel_->setShowOffline(showOffline);
                         this->updateStatus();
                     });
    QObject::connect(this->list_, &QAbstractItemView::activated, this,
                     [this](const QModelIndex &index) {
                         this->openChannelInNewTab(index.sibling(index.row(), 0)
                                                       .data(CHANNEL_LOGIN_ROLE)
                                                       .toString());
                     });
    QObject::connect(this->list_, &QTableView::customContextMenuRequested, this,
                     &FollowedChannelsWindow::showContextMenu);
    QObject::connect(this->refreshTimer_, &QTimer::timeout, this, [this] {
        const auto account = getApp()->getAccounts()->twitch.getCurrent();
        if (!account->isAnon())
        {
            this->loadFollowedStreams(account->getUserId());
        }
    });
    QObject::connect(
        this->followedChannelsRefreshTimer_, &QTimer::timeout, this, [this] {
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

    this->refreshTimer_->start(std::chrono::minutes(1));
    this->followedChannelsRefreshTimer_->start(std::chrono::minutes(5));
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
             this->search_->setFocus();
             this->search_->selectAll();
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
    this->followedChannelsToken_ = CancellationToken{};
    this->streamsToken_ = CancellationToken{};
    this->followedChannels_.clear();
    this->streams_.clear();
    this->followedChannelsError_.clear();
    this->streamsError_.clear();
    this->loadingFollowedChannels_ = false;
    this->loadingStreams_ = false;
    this->model_->removeRows(0, this->model_->rowCount());

    const auto account = getApp()->getAccounts()->twitch.getCurrent();
    if (account->isAnon())
    {
        this->status_->setText("Log in to Twitch to see followed channels.");
        this->status_->show();
        return;
    }

    this->status_->setText("Loading followed channels...");
    this->status_->show();
    this->loadFollowedChannels(account->getUserId());
    this->loadFollowedStreams(account->getUserId());
}

void FollowedChannelsWindow::loadFollowedChannels(const QString &userID)
{
    this->loadingFollowedChannels_ = true;
    this->followedChannelsError_.clear();
    this->pendingFollowedChannels_.clear();
    CancellationToken token(false);
    this->followedChannelsToken_ = token;
    const auto requestToken = token;
    QPointer self(this);
    getHelix()->getFollowedChannels(
        userID,
        [this](const auto &channels, const auto &state) {
            for (const auto &channel : channels)
            {
                this->pendingFollowedChannels_.insert_or_assign(
                    channel.broadcasterID, channel);
            }
            if (state.done)
            {
                this->loadingFollowedChannels_ = false;
                this->followedChannels_.swap(this->pendingFollowedChannels_);
                this->updateList();
            }
        },
        [self, requestToken](const QString &error) {
            if (self == nullptr || requestToken.isCancelled())
            {
                return;
            }
            self->loadingFollowedChannels_ = false;
            self->followedChannelsError_ = error;
            self->updateList();
        },
        std::move(token));
}

void FollowedChannelsWindow::loadFollowedStreams(const QString &userID)
{
    this->loadingStreams_ = true;
    this->streamsError_.clear();
    this->pendingStreams_.clear();
    CancellationToken token(false);
    this->streamsToken_ = token;
    const auto requestToken = token;
    QPointer self(this);
    getHelix()->getFollowedStreams(
        userID,
        [this](const auto &streams, const auto &state) {
            for (const auto &stream : streams)
            {
                this->pendingStreams_.insert_or_assign(stream.userId, stream);
            }
            if (state.done)
            {
                this->loadingStreams_ = false;
                this->streams_.swap(this->pendingStreams_);
                this->updateList();
            }
        },
        [self, requestToken](const QString &error) {
            if (self == nullptr || requestToken.isCancelled())
            {
                return;
            }
            self->loadingStreams_ = false;
            self->streamsError_ = error;
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
    entries.reserve(this->streams_.size() + this->followedChannels_.size());
    for (const auto &[id, stream] : this->streams_)
    {
        entries.push_back({id, stream.userLogin, stream.userName, &stream});
    }
    for (const auto &[id, channel] : this->followedChannels_)
    {
        if (!this->streams_.contains(id))
        {
            entries.push_back({id, channel.broadcasterLogin,
                               channel.broadcasterName, nullptr});
        }
    }

    QHash<QString, QStandardItem *> existingRows;
    for (int row = 0; row < this->model_->rowCount(); ++row)
    {
        auto *item = this->model_->item(row, 0);
        existingRows.insert(item->data(CHANNEL_ID_ROLE).toString(), item);
    }

    QSet<QString> currentIDs;
    QList<QStandardItem *> items;
    items.reserve(this->model_->columnCount());
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
            this->model_->appendRow(items);
        }
        const auto row = existing == existingRows.end()
                             ? this->model_->rowCount() - 1
                             : existing.value()->row();
        for (int column = 0; column < columns.size(); ++column)
        {
            auto *item = this->model_->item(row, column);
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
        auto *first = this->model_->item(row, 0);
        first->setData(entry.id, CHANNEL_ID_ROLE);
        first->setData(entry.login, CHANNEL_LOGIN_ROLE);
        first->setData(stream != nullptr, LIVE_ROLE);
    }

    for (int row = this->model_->rowCount() - 1; row >= 0; --row)
    {
        if (!currentIDs.contains(
                this->model_->item(row, 0)->data(CHANNEL_ID_ROLE).toString()))
        {
            this->model_->removeRow(row);
        }
    }

    this->updateStatus();
}

void FollowedChannelsWindow::updateStatus()
{
    const auto query = this->search_->text().trimmed();
    const auto account = getApp()->getAccounts()->twitch.getCurrent();
    if (account->isAnon())
    {
        this->status_->setText("Log in to Twitch to see followed channels.");
    }
    else if (!this->streamsError_.isEmpty())
    {
        this->status_->setText("Failed to load followed streams: " +
                               this->streamsError_);
    }
    else if (this->showOffline_->isChecked() &&
             !this->followedChannelsError_.isEmpty())
    {
        this->status_->setText("Failed to load followed channels: " +
                               this->followedChannelsError_);
    }
    else if (this->loadingStreams_ || (this->showOffline_->isChecked() &&
                                       this->loadingFollowedChannels_))
    {
        this->status_->setText("Loading followed channels...");
    }
    else if (this->proxyModel_->rowCount() == 0)
    {
        if (!query.isEmpty())
        {
            this->status_->setText("No matching channels.");
        }
        else if (this->showOffline_->isChecked())
        {
            this->status_->setText("No followed channels found.");
        }
        else
        {
            this->status_->setText("No followed channels are live.");
        }
    }
    else
    {
        this->status_->clear();
    }
    this->status_->setVisible(!this->status_->text().isEmpty());
}

void FollowedChannelsWindow::openChannelInNewTab(const QString &channelLogin)
{
    if (channelLogin.isEmpty())
    {
        return;
    }

    auto *tab = this->window_->getNotebook().addPage(true);
    tab->appendNewSplit(false)->setChannel(
        getApp()->getTwitch()->getOrAddChannel(channelLogin));
}

void FollowedChannelsWindow::openChannelInNewSplit(const QString &channelLogin)
{
    if (channelLogin.isEmpty())
    {
        return;
    }

    this->window_->getNotebook()
        .getOrAddSelectedPage()
        ->appendNewSplit(false)
        ->setChannel(getApp()->getTwitch()->getOrAddChannel(channelLogin));
}

QString FollowedChannelsWindow::selectedChannel() const
{
    const auto index = this->list_->currentIndex();
    if (!index.isValid())
    {
        return {};
    }
    return index.sibling(index.row(), 0).data(CHANNEL_LOGIN_ROLE).toString();
}

void FollowedChannelsWindow::showContextMenu(QPoint position)
{
    this->list_->setCurrentIndex(this->list_->indexAt(position));
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
    menu->popup(this->list_->viewport()->mapToGlobal(position));
}

}  // namespace chatterino
