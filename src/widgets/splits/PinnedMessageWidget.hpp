// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BaseWidget.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QDateTime>
#include <QTimer>

class QLabel;
class QTextEdit;

namespace chatterino {

class TwitchChannel;
class DrawnButton;

/**
 * Banner shown between the split header and the chat view that
 * displays the channel's currently pinned message.
 */
class PinnedMessageWidget final : public BaseWidget
{
    Q_OBJECT

public:
    explicit PinnedMessageWidget(QWidget *parent = nullptr);

    // Pass nullptr to detach from any channel.
    void setChannel(TwitchChannel *channel);

    // Called by the header pin button to toggle manual visibility.
    void toggleUserPinned();

    /// Emitted whenever this widget becomes shown or hidden.
    pajlada::Signals::NoArgSignal visibilityChanged;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void scaleChangedEvent(float newScale) override;

private:
    void paintEvent(QPaintEvent *event) override;
    void refresh();
    void showModMenu();
    void tickProgress();

    TwitchChannel *channel_ = nullptr;
    pajlada::Signals::SignalHolder signalHolder_;

    // Header row
    QLabel *pinnedByLabel_ = nullptr;
    QLabel *countdownLabel_ = nullptr;
    DrawnButton *menuButton_{};  // mod menu

    // Body
    QTextEdit *messageLabel_ = nullptr;
    QLabel *footerLabel_ = nullptr;

    QTimer *progressTimer_ = nullptr;
    QTimer *autoHideTimer_ = nullptr;
    bool userToggled_ = false;  // true while user manually pinned the widget
    QDateTime pinEndsAt_;       // invalid when no end time
};

}  // namespace chatterino
