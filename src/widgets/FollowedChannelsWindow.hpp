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
class QStandardItemModel;
class QTableView;
class QTimer;

namespace chatterino {

class FollowedChannelsFilterModel;
class Window;

class FollowedChannelsWindow : public BasePopup
{
    Q_OBJECT

public:
    explicit FollowedChannelsWindow(Window *parent);

private:
    void addShortcuts() override;
    void themeChangedEvent() override;
    void load();
    void loadFollowedChannels(const QString &userID);
    void loadFollowedStreams(const QString &userID);
    void updateList();
    void updateStatus();
    void openChannelInNewTab(const QString &channelLogin);
    void openChannelInNewSplit(const QString &channelLogin);
    void showContextMenu(QPoint position);
    QString selectedChannel() const;

    struct {
        QLineEdit *search;
        QCheckBox *showOffline;
        QLabel *status;
        QStandardItemModel *model;
        FollowedChannelsFilterModel *proxyModel;
        QTableView *list;
    } ui;

    // Refresh followed live channels every minute
    QTimer *refreshTimer{};
    // Refresh the complete followed-channel list every five minutes
    QTimer *followedChannelsRefreshTimer{};

    // Open tabs and splits in the window that opened this popup
    Window *window{};

    // The last complete followed-channel list
    std::map<QString, HelixFollowedChannel> followedChannels;
    // Pages collected so far for the current followed-channel request
    std::map<QString, HelixFollowedChannel> pendingFollowedChannels;

    // The last complete list of followed live channels
    std::map<QString, HelixStream> streams;
    // Pages collected so far for the current request for followed live channels
    std::map<QString, HelixStream> pendingStreams;

    // Cancels the followed-channel request when replaced or destroyed
    ScopedCancellationToken followedChannelsToken;
    // Cancels the request for followed live channels when replaced or destroyed
    ScopedCancellationToken streamsToken;

    // Error from the latest followed-channel request
    QString followedChannelsError;
    // Error from the latest request for followed live channels
    QString streamsError;

    // Whether a followed-channel request is in progress
    bool loadingFollowedChannels{};
    // Whether a request for followed live channels is in progress
    bool loadingStreams{};
};

}  // namespace chatterino
