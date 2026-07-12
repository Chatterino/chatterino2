// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/BaseWidget.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QDateTime>
#include <QTimer>

#include <memory>

class QLabel;
class QScrollArea;
class QMenu;

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
    void resizeEvent(QResizeEvent *event) override;
    void scaleChangedEvent(float newScale) override;

private:
    void paintEvent(QPaintEvent *event) override;
    void refresh();
    /// Builds the moderator menu shown when clicking the menu button.
    std::unique_ptr<QMenu> buildModMenu();
    void tickProgress();
    /// Sizes the message scroll area to its wrapped content, capped at the
    /// (scaled) maximum height. A vertical scrollbar appears past the cap.
    void updateMessageHeight();

    TwitchChannel *channel_ = nullptr;
    pajlada::Signals::SignalHolder signalHolder_;

    // Header row
    QLabel *pinnedByLabel_ = nullptr;
    QLabel *countdownLabel_ = nullptr;
    /// Mod Menu.
    DrawnButton *menuButton_ = nullptr;

    // Body
    QScrollArea *messageScrollArea_ = nullptr;
    QLabel *messageLabel_ = nullptr;
    QLabel *footerLabel_ = nullptr;

    QTimer *progressTimer_ = nullptr;
    QTimer *autoHideTimer_ = nullptr;
    /// Scaled cap for the message body.
    int messageMaxHeight_ = 110;
    /// True while user manually pinned the widget.
    bool userToggled_ = false;
    /// Invalid when no end time.
    QDateTime pinEndsAt_;
};

}  // namespace chatterino
