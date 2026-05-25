// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/splits/PinnedMessageWidget.hpp"

#include "Application.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

#include "widgets/buttons/LabelButton.hpp"

#include "singletons/Theme.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QAbstractTextDocumentLayout>
#include <QPainter>
#include <QPaintEvent>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include <chrono>
#include <optional>

namespace chatterino {

namespace {

constexpr auto MUTED_STYLE = "color: #adadb8;";

}  // namespace

PinnedMessageWidget::PinnedMessageWidget(QWidget *parent)
    : QFrame(parent)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    this->setAttribute(Qt::WA_OpaquePaintEvent);

    auto *outerBox = new QVBoxLayout(this);
    outerBox->setContentsMargins(0, 0, 0, 0);
    outerBox->setSpacing(0);

    auto *contentBox = new QVBoxLayout();
    contentBox->setContentsMargins(8, 6, 8, 6);
    contentBox->setSpacing(3);

    // Header row: "Pinned by <user>"  [⋮]
    auto *headerRow = new QHBoxLayout();
    headerRow->setSpacing(4);

    this->pinnedByLabel_ = new QLabel(this);
    {
        QFont f = this->pinnedByLabel_->font();
        f.setPointSize(11);
        this->pinnedByLabel_->setFont(f);
    }
    headerRow->addWidget(this->pinnedByLabel_);
    headerRow->addStretch(1);
    this->menuButton_ = new LabelButton(QStringLiteral("\u205D"));
    this->menuButton_->setToolTip(QStringLiteral("Mod options"));
    this->menuButton_->setFixedSize(30, 22);
    this->menuButton_->hide();
    headerRow->addWidget(this->menuButton_);

    contentBox->addLayout(headerRow);

    // Message body
    this->messageLabel_ = new QTextEdit(this);
    this->messageLabel_->setReadOnly(true);
    this->messageLabel_->setFocusPolicy(Qt::NoFocus);
    this->messageLabel_->setFrameShape(QFrame::NoFrame);
    this->messageLabel_->setStyleSheet("QTextEdit { background: transparent; }");
    this->messageLabel_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->messageLabel_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->messageLabel_->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    this->messageLabel_->setMaximumHeight(110);
    this->messageLabel_->setSizePolicy(QSizePolicy::Expanding,
                                       QSizePolicy::Fixed);
    this->messageLabel_->document()->setDocumentMargin(0);
    this->messageLabel_->setContentsMargins(0, 0, 0, 0);
    {
        QFont f = this->messageLabel_->font();
        f.setPointSize(13);
        this->messageLabel_->setFont(f);
    }
    QObject::connect(
        this->messageLabel_->document()->documentLayout(),
        &QAbstractTextDocumentLayout::documentSizeChanged,
        this->messageLabel_,
        [label = this->messageLabel_](const QSizeF &newSize) {
            const int h =
                qBound(1, (int)std::ceil(newSize.height()), 110);
            label->setFixedHeight(h);
        });
    contentBox->addWidget(this->messageLabel_);

    // Footer: [sender · time] ... [countdown]
    auto *footerRow = new QHBoxLayout();
    footerRow->setContentsMargins(0, 2, 0, 0);
    footerRow->setSpacing(4);

    this->footerLabel_ = new QLabel(this);
    this->footerLabel_->setStyleSheet(MUTED_STYLE);
    {
        QFont f = this->footerLabel_->font();
        f.setPointSize(10);
        this->footerLabel_->setFont(f);
    }
    footerRow->addWidget(this->footerLabel_);
    footerRow->addStretch(1);

    this->countdownLabel_ = new QLabel(this);
    this->countdownLabel_->setStyleSheet(MUTED_STYLE);
    {
        QFont f = this->countdownLabel_->font();
        f.setPointSize(10);
        this->countdownLabel_->setFont(f);
    }
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
    this->progressTimer_ = new QTimer(this);
    this->progressTimer_->setInterval(1000);
    QObject::connect(this->progressTimer_, &QTimer::timeout, this,
                     [this] {
                         this->tickProgress();
                     });

    // auto-hide timer
    this->autoHideTimer_ = new QTimer(this);
    this->autoHideTimer_->setSingleShot(true);
    QObject::connect(this->autoHideTimer_, &QTimer::timeout, this, [this] {
        if (!this->userToggled_)
        {
            this->hide();
        }
    });

    QObject::connect(this->menuButton_, &Button::leftClicked, this,
                     [this] {
                         this->showModMenu();
                     });

    this->hide();
}

void PinnedMessageWidget::tickProgress()
{
    const qint64 nowMs = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
    const qint64 endsMs = this->pinEndsAt_.toMSecsSinceEpoch();

    if (nowMs >= endsMs)
    {
        this->progressTimer_->stop();
        this->countdownLabel_->hide();
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
        timeStr = QStringLiteral("\u23F1 %1:%2:%3")
                      .arg(hours)
                      .arg(mins, 2, 10, QLatin1Char('0'))
                      .arg(secs, 2, 10, QLatin1Char('0'));
    }
    else
    {
        timeStr = QStringLiteral("\u23F1 %1:%2")
                      .arg(mins, 2, 10, QLatin1Char('0'))
                      .arg(secs, 2, 10, QLatin1Char('0'));
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
    }

    this->refresh();
}

void PinnedMessageWidget::showModMenu()
{
    if (!this->channel_)
    {
        return;
    }

    QMenu menu(this);

    menu.addAction(QStringLiteral("Unpin this Message"), [this] {
        if (this->channel_)
        {
            this->channel_->unpinCurrentMessage();
        }
    });

    auto *unpinAfterMenu = menu.addMenu(QStringLiteral("Unpin After"));

    const auto addDuration = [&](const QString &label,
                                 std::optional<int> seconds) {
        unpinAfterMenu->addAction(label, [this, seconds] {
            if (!this->channel_)
            {
                return;
            }
            const auto pin = this->channel_->getPinnedMessage();
            if (!pin)
            {
                return;
            }
            auto currentAccount =
                getApp()->getAccounts()->twitch.getCurrent();
            if (!currentAccount || currentAccount->isAnon())
            {
                return;
            }
            std::optional<std::chrono::seconds> duration =
                seconds ? std::make_optional(std::chrono::seconds(*seconds))
                        : std::nullopt;
            this->channel_->updatePinnedMessageAs(pin->messageID, duration,
                                                  *currentAccount,
                                                  pin->messageText);
        });
    };

    addDuration(QStringLiteral("1 minute"), 60);
    addDuration(QStringLiteral("5 minutes"), 300);
    addDuration(QStringLiteral("10 minutes"), 600);
    addDuration(QStringLiteral("20 minutes"), 1200);
    addDuration(QStringLiteral("30 minutes"), 1800);
    unpinAfterMenu->addSeparator();
    addDuration(QStringLiteral("Manually unpinned or end of stream"),
                std::nullopt);

    menu.addSeparator();

    menu.addAction(QStringLiteral("Hide for Yourself"), [this] {
        this->hide();
    });

    menu.exec(this->menuButton_->mapToGlobal(
        QPoint(0, this->menuButton_->height())));
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

    const auto pin = this->channel_->getPinnedMessage();
    if (!pin)
    {
        this->progressTimer_->stop();
        this->autoHideTimer_->stop();
        this->userToggled_ = false;
        this->hide();
        return;
    }

    this->pinnedByLabel_->setText(
        QStringLiteral("Pinned by <b>%1</b>")
            .arg(pin->pinnedBy.displayName.toHtmlEscaped()));

    this->messageLabel_->setPlainText(pin->messageText);

    {
        const QString sentAt =
            pin->startsAt.toLocalTime().toString(QStringLiteral("h:mm AP"));
        this->footerLabel_->setText(
            QStringLiteral("Sent by %1 \u00B7 %2")
                .arg(pin->sender.displayName.toHtmlEscaped(), sentAt));
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
        this->autoHideTimer_->start(5000);
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

bool PinnedMessageWidget::hasPinnedMessage() const
{
    return this->channel_ && this->channel_->getPinnedMessage() != nullptr;
}

}  // namespace chatterino
