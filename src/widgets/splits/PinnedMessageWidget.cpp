// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/splits/PinnedMessageWidget.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "widgets/buttons/DrawnButton.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QScrollArea>
#include <QShowEvent>
#include <QVBoxLayout>

using namespace std::chrono_literals;
using namespace Qt::Literals;

#include <chrono>
#include <memory>
#include <optional>

namespace chatterino {

PinnedMessageWidget::PinnedMessageWidget(QWidget *parent)
    : SplitBanner(parent)
    , menuButton_(new DrawnButton(DrawnButton::Symbol::Kebab, {}, this))
    , messageScrollArea_(new QScrollArea(this))
    , messageLabel_(new QLabel(this))
    , footerLabel_(new QLabel(this))
{
    // Header row: "Pinned by <user>"  [⋮]
    this->menuButton_->setScaleIndependentSize(28, 28);
    this->menuButton_->setToolTip(u"Mod options"_s);
    this->menuButton_->setMenu(this->buildModMenu());
    this->menuButton_->hide();
    this->headerRow()->addWidget(this->menuButton_);

    // Message body
    this->messageLabel_->setWordWrap(true);
    this->messageLabel_->setTextFormat(Qt::PlainText);
    this->messageLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    this->messageLabel_->setStyleSheet("background: transparent;");
    this->messageLabel_->setSizePolicy(QSizePolicy::Expanding,
                                       QSizePolicy::Preferred);
    this->messageLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    this->messageScrollArea_->setWidgetResizable(true);
    this->messageScrollArea_->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);
    this->messageScrollArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->messageScrollArea_->setFrameShape(QFrame::NoFrame);
    this->messageScrollArea_->setFocusPolicy(Qt::NoFocus);
    this->messageScrollArea_->setStyleSheet(
        "QScrollArea { background: transparent; } "
        "QScrollArea > QWidget > QWidget { background: transparent; }");
    this->messageScrollArea_->viewport()->setAutoFillBackground(false);
    this->messageScrollArea_->setSizePolicy(QSizePolicy::Expanding,
                                            QSizePolicy::Fixed);
    this->messageScrollArea_->setWidget(this->messageLabel_);
    this->contentBox()->addWidget(this->messageScrollArea_);

    // Footer: [sender · time] ... [countdown]
    auto *footerRow = new QHBoxLayout();
    footerRow->setContentsMargins(0, 2, 0, 0);
    footerRow->setSpacing(4);

    this->footerLabel_->setStyleSheet(MUTED_STYLE);
    footerRow->addWidget(this->footerLabel_);
    footerRow->addStretch(1);

    footerRow->addWidget(this->countdownLabel());
    this->contentBox()->addLayout(footerRow);

    this->scaleChangedEvent(this->scale());
    this->hide();
}

void PinnedMessageWidget::tickCountdown()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 endsMs = this->pinEndsAt_.toMSecsSinceEpoch();

    if (nowMs >= endsMs)
    {
        this->stopCountdown();
        this->countdownLabel()->hide();
        if (this->channel_)
        {
            this->channel_->clearPinnedMessage();
        }
        return;
    }

    this->countdownLabel()->setText(
        u"\u23F1 %1"_s.arg(formatCountdown(endsMs - nowMs)));
    this->countdownLabel()->show();
}

void PinnedMessageWidget::autoHide()
{
    if (!this->userToggled_)
    {
        this->hide();
    }
}

void PinnedMessageWidget::setChannel(TwitchChannel *channel)
{
    this->signalHolder_.clear();
    this->channel_ = channel;
    this->userToggled_ = false;
    this->stopAutoHide();

    if (channel)
    {
        this->signalHolder_.managedConnect(channel->pinnedMessageChanged,
                                           [this] {
                                               this->userToggled_ = false;
                                               this->refresh();
                                           });
        this->signalHolder_.managedConnect(channel->userStateChanged, [this] {
            this->refresh();
        });
    }

    this->refresh();
}

std::unique_ptr<QMenu> PinnedMessageWidget::buildModMenu()
{
    auto menu = std::make_unique<QMenu>(this);

    menu->addAction(u"Unpin this Message"_s, this, [this] {
        if (this->channel_)
        {
            this->channel_->unpinCurrentMessage();
        }
    });

    auto *unpinAfterMenu = menu->addMenu(u"Unpin After"_s);

    const auto addDuration = [&](const QString &label,
                                 std::optional<std::chrono::seconds> duration) {
        unpinAfterMenu->addAction(label, this, [this, duration] {
            if (!this->channel_)
            {
                return;
            }
            const auto *pin = this->channel_->getPinnedMessage();
            if (!pin)
            {
                return;
            }
            auto currentAccount = getApp()->getAccounts()->twitch.getCurrent();
            if (!currentAccount || currentAccount->isAnon())
            {
                return;
            }
            this->channel_->updatePinnedMessageAs(
                pin->messageID, duration, *currentAccount, pin->messageText);
        });
    };

    addDuration(u"1 minute"_s, 1min);
    addDuration(u"5 minutes"_s, 5min);
    addDuration(u"10 minutes"_s, 10min);
    addDuration(u"20 minutes"_s, 20min);
    addDuration(u"30 minutes"_s, 30min);
    unpinAfterMenu->addSeparator();
    addDuration(u"End of stream"_s, std::nullopt);

    menu->addSeparator();

    menu->addAction(u"Hide for Yourself"_s, this, [this] {
        this->hide();
    });

    return menu;
}

void PinnedMessageWidget::refresh()
{
    if (!this->channel_)
    {
        this->stopCountdown();
        this->stopAutoHide();
        this->userToggled_ = false;
        this->hide();
        return;
    }

    const auto *pin = this->channel_->getPinnedMessage();
    if (!pin)
    {
        this->stopCountdown();
        this->stopAutoHide();
        this->userToggled_ = false;
        this->hide();
        return;
    }

    const auto mode = static_cast<UsernameDisplayMode>(
        getSettings()->usernameDisplayMode.getValue());
    this->headerLabel()->setText(u"Pinned by <b>%1</b>"_s.arg(
        pin->pinnedBy.formatted(mode).toHtmlEscaped()));

    this->messageLabel_->setText(pin->messageText);
    this->updateMessageHeight();

    {
        const QString sentAt = pin->startsAt.toLocalTime().toString(
            getSettings()->timestampFormat);
        this->footerLabel_->setText(u"Sent by %1 \u00B7 %2"_s.arg(
            pin->sender.formatted(mode).toHtmlEscaped(), sentAt));
    }

    this->stopCountdown();
    this->countdownLabel()->hide();
    if (pin->endsAt.has_value() && pin->endsAt->isValid())
    {
        this->pinEndsAt_ = *pin->endsAt;
        this->startCountdown();
    }

    const bool isMod = this->channel_->hasModRights();
    this->menuButton_->setVisible(isMod);

    this->show();
    this->updateMessageHeightIfNeeded();

    this->stopAutoHide();
    if (!getSettings()->alwaysShowPinnedMessage && !this->userToggled_)
    {
        this->startAutoHide(30s);
    }
}

void PinnedMessageWidget::toggleUserPinned()
{
    if (this->isVisible())
    {
        this->userToggled_ = false;
        this->stopAutoHide();
        this->hide();
    }
    else
    {
        this->userToggled_ = true;
        this->stopAutoHide();
        this->show();
        this->updateMessageHeightIfNeeded();
    }
}

void PinnedMessageWidget::updateMessageHeight()
{
    if (!this->messageLabel_ || !this->messageScrollArea_)
    {
        return;
    }

    // Wrapped height of the label at the current viewport width.
    this->lastViewportWidth_ = this->messageScrollArea_->viewport()->width();
    int contentH =
        this->messageLabel_->heightForWidth(this->lastViewportWidth_);
    if (contentH <= 0)
    {
        contentH = this->messageLabel_->sizeHint().height();
    }

    // Size to content, but never taller than the cap.
    this->messageScrollArea_->setFixedHeight(
        qBound(1, contentH, this->messageMaxHeight_));
}

void PinnedMessageWidget::updateMessageHeightIfNeeded()
{
    if (this->lastViewportWidth_ !=
        this->messageScrollArea_->viewport()->width())
    {
        this->updateMessageHeight();
    }
}

void PinnedMessageWidget::resizeEvent(QResizeEvent *event)
{
    SplitBanner::resizeEvent(event);
    this->updateMessageHeight();
}

void PinnedMessageWidget::showEvent(QShowEvent *event)
{
    SplitBanner::showEvent(event);
    this->visibilityChanged.invoke();
}

void PinnedMessageWidget::hideEvent(QHideEvent *event)
{
    SplitBanner::hideEvent(event);
    this->visibilityChanged.invoke();
}

void PinnedMessageWidget::scaleChangedEvent(float newScale)
{
    SplitBanner::scaleChangedEvent(newScale);

    QFont bodyFont = this->messageLabel_->font();
    bodyFont.setPointSizeF(11.0F * newScale);
    this->messageLabel_->setFont(bodyFont);
    this->messageMaxHeight_ = int(110 * newScale);
    this->updateMessageHeight();

    QFont footerFont = this->footerLabel_->font();
    footerFont.setPointSizeF(9.0F * newScale);
    this->footerLabel_->setFont(footerFont);
}

}  // namespace chatterino
