// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <pajlada/signals/signalholder.hpp>
#include <QDateTime>
#include <QFrame>
#include <QTimer>

namespace chatterino {

class TwitchChannel;

/**
 * Banner shown between the split header and the chat view that
 * displays the channel's currently pinned message.
 */
class PinnedMessageWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit PinnedMessageWidget(QWidget *parent = nullptr);

    // Pass nullptr to detach from any channel.
    void setChannel(TwitchChannel *channel);

    // Called by the header pin button to toggle manual visibility.
    void toggleUserPinned();

    [[nodiscard]] bool hasPinnedMessage() const;

private:
    void paintEvent(QPaintEvent *event) override;
    void refresh();
    void showModMenu();
    void tickProgress();

    TwitchChannel *channel_ = nullptr;
    pajlada::Signals::SignalHolder signalHolder_;

    // Header row
    class QLabel *pinnedByLabel_;
    class QLabel *countdownLabel_;
    class LabelButton *menuButton_;  // ⁝ mod menu

    // Body
    class QTextEdit *messageLabel_;
    class QLabel *footerLabel_;

    QTimer *progressTimer_ = nullptr;
    QTimer *autoHideTimer_ = nullptr;
    bool userToggled_ = false;  // true while user manually pinned the widget
    QDateTime pinEndsAt_;  // invalid when no end time
};

}  // namespace chatterino
