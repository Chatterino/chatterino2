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
#include "singletons/Theme.hpp"
#include "widgets/buttons/DrawnButton.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollArea>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

using namespace std::chrono_literals;
using namespace Qt::Literals;

#include <chrono>
#include <memory>
#include <optional>

namespace chatterino {

namespace {

constexpr auto MUTED_STYLE = "color: #adadb8;";

}  // namespace

PinnedMessageWidget::PinnedMessageWidget(QWidget *parent)
    : BaseWidget(parent)
    , pinnedByLabel_(new QLabel(this))
    , countdownLabel_(new QLabel(this))
    , menuButton_(new DrawnButton(DrawnButton::Symbol::Kebab, {}, this))
    , messageScrollArea_(new QScrollArea(this))
    , messageLabel_(new QLabel(this))
    , footerLabel_(new QLabel(this))
    , progressTimer_(new QTimer(this))
    , autoHideTimer_(new QTimer(this))
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto *outerBox = new QVBoxLayout(this);
    outerBox->setContentsMargins(0, 0, 0, 0);
    outerBox->setSpacing(0);

    auto *contentBox = new QVBoxLayout();
    contentBox->setContentsMargins(8, 6, 8, 6);
    contentBox->setSpacing(3);

    // Header row: "Pinned by <user>"  [⋮]
    auto *headerRow = new QHBoxLayout();
    headerRow->setSpacing(4);

    headerRow->addWidget(this->pinnedByLabel_);
    headerRow->addStretch(1);
    this->menuButton_->setScaleIndependentSize(28, 28);
    this->menuButton_->setToolTip(u"Mod options"_s);
    this->menuButton_->setMenu(this->buildModMenu());
    this->menuButton_->hide();
    headerRow->addWidget(this->menuButton_);

    contentBox->addLayout(headerRow);

    // Message body
    this->messageLabel_->setWordWrap(true);
    this->messageLabel_->setTextFormat(Qt::PlainText);
    this->messageLabel_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    this->messageLabel_->setStyleSheet("background: transparent;");
    this->messageLabel_->setSizePolicy(QSizePolicy::Expanding,
                                       QSizePolicy::Preferred);

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
    contentBox->addWidget(this->messageScrollArea_);

    // Footer: [sender · time] ... [countdown]
    auto *footerRow = new QHBoxLayout();
    footerRow->setContentsMargins(0, 2, 0, 0);
    footerRow->setSpacing(4);

    this->footerLabel_->setStyleSheet(MUTED_STYLE);
    footerRow->addWidget(this->footerLabel_);
    footerRow->addStretch(1);

    this->countdownLabel_->setStyleSheet(MUTED_STYLE);
    this->countdownLabel_->hide();
    footerRow->addWidget(this->countdownLabel_);
    contentBox->addLayout(footerRow);

    outerBox->addLayout(contentBox);

    // 1px bottom border - separates pin widget from the chat view below
    auto *bottomBorder = new QWidget(this);
    bottomBorder->setFixedHeight(1);
    bottomBorder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    bottomBorder->setAutoFillBackground(true);
    {
        QPalette pal = bottomBorder->palette();
        pal.setColor(QPalette::Window, pal.color(QPalette::Mid));
        bottomBorder->setPalette(pal);
    }
    outerBox->addWidget(bottomBorder);

    // Countdown timer (fires every second)
    this->progressTimer_->setInterval(1s);
    QObject::connect(this->progressTimer_, &QTimer::timeout, this, [this] {
        this->tickProgress();
    });

    // auto-hide timer
    this->autoHideTimer_->setSingleShot(true);
    QObject::connect(this->autoHideTimer_, &QTimer::timeout, this, [this] {
        if (!this->userToggled_)
        {
            this->hide();
        }
    });

    this->scaleChangedEvent(this->scale());
    this->hide();
}

void PinnedMessageWidget::tickProgress()
{
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 endsMs = this->pinEndsAt_.toMSecsSinceEpoch();

    if (nowMs >= endsMs)
    {
        this->progressTimer_->stop();
        this->countdownLabel_->hide();
        if (this->channel_)
        {
            this->channel_->clearPinnedMessage();
        }
        return;
    }

    const qint64 remainingMs = endsMs - nowMs;
    const qint64 totalSecs = (remainingMs + 999) / 1000;  // round up
    const qint64 hours = totalSecs / 3600;
    const qint64 mins = (totalSecs % 3600) / 60;
    const qint64 secs = totalSecs % 60;

    QString timeStr;
    if (hours > 0)
    {
        timeStr = u"\u23F1 %1:%2:%3"_s.arg(hours)
                      .arg(mins, 2, 10, QChar(u'0'))
                      .arg(secs, 2, 10, QChar(u'0'));
    }
    else
    {
        timeStr = u"\u23F1 %1:%2"_s.arg(mins, 2, 10, QChar(u'0'))
                      .arg(secs, 2, 10, QChar(u'0'));
    }

    this->countdownLabel_->setText(timeStr);
    this->countdownLabel_->show();
}

void PinnedMessageWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    auto *theme = getTheme();

    // Fill background (same color as the split header above)
    painter.fillRect(event->rect(), theme->splits.header.background);

    // Draw 1px top border
    painter.setPen(theme->splits.header.border);
    painter.drawLine(0, 0, this->width() - 1, 0);
}

void PinnedMessageWidget::setChannel(TwitchChannel *channel)
{
    this->signalHolder_.clear();
    this->channel_ = channel;
    this->userToggled_ = false;
    this->autoHideTimer_->stop();

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
        this->progressTimer_->stop();
        this->autoHideTimer_->stop();
        this->userToggled_ = false;
        this->hide();
        return;
    }

    const auto *pin = this->channel_->getPinnedMessage();
    if (!pin)
    {
        this->progressTimer_->stop();
        this->autoHideTimer_->stop();
        this->userToggled_ = false;
        this->hide();
        return;
    }

    const auto mode = static_cast<UsernameDisplayMode>(
        getSettings()->usernameDisplayMode.getValue());
    this->pinnedByLabel_->setText(u"Pinned by <b>%1</b>"_s.arg(
        pin->pinnedBy.formatted(mode).toHtmlEscaped()));

    this->messageLabel_->setText(pin->messageText);
    this->updateMessageHeight();

    {
        const QString sentAt = pin->startsAt.toLocalTime().toString(
            getSettings()->timestampFormat);
        this->footerLabel_->setText(u"Sent by %1 \u00B7 %2"_s.arg(
            pin->sender.formatted(mode).toHtmlEscaped(), sentAt));
    }

    this->progressTimer_->stop();
    this->countdownLabel_->hide();
    if (pin->endsAt.has_value() && pin->endsAt->isValid())
    {
        this->pinEndsAt_ = *pin->endsAt;
        this->tickProgress();  // set initial text immediately
        this->progressTimer_->start();
    }

    const bool isMod = this->channel_->hasModRights();
    this->menuButton_->setVisible(isMod);

    this->show();

    this->autoHideTimer_->stop();
    if (!getSettings()->alwaysShowPinnedMessage && !this->userToggled_)
    {
        this->autoHideTimer_->start(30s);
    }
}

void PinnedMessageWidget::toggleUserPinned()
{
    if (this->isVisible())
    {
        this->userToggled_ = false;
        this->autoHideTimer_->stop();
        this->hide();
    }
    else
    {
        this->userToggled_ = true;
        this->autoHideTimer_->stop();
        this->show();
    }
}

void PinnedMessageWidget::updateMessageHeight()
{
    if (!this->messageLabel_ || !this->messageScrollArea_)
    {
        return;
    }

    // Wrapped height of the label at the current viewport width.
    const int width = this->messageScrollArea_->viewport()->width();
    int contentH = this->messageLabel_->heightForWidth(width);
    if (contentH <= 0)
    {
        contentH = this->messageLabel_->sizeHint().height();
    }

    // Size to content, but never taller than the cap.
    this->messageScrollArea_->setFixedHeight(
        qBound(1, contentH, this->messageMaxHeight_));
}

void PinnedMessageWidget::resizeEvent(QResizeEvent *event)
{
    BaseWidget::resizeEvent(event);
    this->updateMessageHeight();
}

void PinnedMessageWidget::showEvent(QShowEvent *event)
{
    BaseWidget::showEvent(event);
    this->visibilityChanged.invoke();
}

void PinnedMessageWidget::hideEvent(QHideEvent *event)
{
    BaseWidget::hideEvent(event);
    this->visibilityChanged.invoke();
}

void PinnedMessageWidget::scaleChangedEvent(float newScale)
{
    QFont headerFont = this->pinnedByLabel_->font();
    headerFont.setPointSizeF(11.0F * newScale);
    this->pinnedByLabel_->setFont(headerFont);
    this->countdownLabel_->setFont(headerFont);

    QFont bodyFont = this->messageLabel_->font();
    bodyFont.setPointSizeF(13.0F * newScale);
    this->messageLabel_->setFont(bodyFont);
    this->messageMaxHeight_ = int(110 * newScale);
    this->updateMessageHeight();

    QFont footerFont = this->footerLabel_->font();
    footerFont.setPointSizeF(10.0F * newScale);
    this->footerLabel_->setFont(footerFont);
}

}  // namespace chatterino
