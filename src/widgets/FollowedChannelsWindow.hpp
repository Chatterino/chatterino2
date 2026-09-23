// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "providers/twitch/api/Helix.hpp"
#include "util/CancellationToken.hpp"
#include "widgets/BasePopup.hpp"

#include <map>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPoint;
class QSortFilterProxyModel;
class QStandardItemModel;
class QTableView;
class QTimer;

namespace chatterino {

class FollowedChannelsWindow : public BasePopup
{
    Q_OBJECT

public:
    explicit FollowedChannelsWindow(QWidget *parent = nullptr);

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void addShortcuts() override;
    void themeChangedEvent() override;
    void load();
    void loadFollowedChannels(const QString &userID);
    void loadFollowedStreams(const QString &userID);
    void updateList(bool preserveViewport = true);
    static void openChannelInNewTab(const QString &channelLogin);
    static void openChannelInNewSplit(const QString &channelLogin);
    void showContextMenu(QPoint position);
    QString selectedChannel() const;

    QLineEdit *search_{};
    QCheckBox *showOffline_{};
    QLabel *status_{};
    QStandardItemModel *model_{};
    QSortFilterProxyModel *proxyModel_{};
    QTableView *list_{};
    QTimer *refreshTimer_{};
    QTimer *followedChannelsRefreshTimer_{};

    std::map<QString, HelixFollowedChannel> followedChannels_;
    std::map<QString, HelixFollowedChannel> pendingFollowedChannels_;
    std::map<QString, HelixStream> streams_;
    std::map<QString, HelixStream> pendingStreams_;
    ScopedCancellationToken followedChannelsToken_;
    ScopedCancellationToken streamsToken_;
    QString followedChannelsError_;
    QString streamsError_;
    bool loadingFollowedChannels_{};
    bool loadingStreams_{};
};

}  // namespace chatterino
