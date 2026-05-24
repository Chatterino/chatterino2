// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/splits/PinnedMessageWidget.hpp"

#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextEdit>
#include <QTimer>
#include <QPalette>
#include <QVBoxLayout>

namespace chatterino {

namespace {

constexpr auto MUTED_STYLE = "color: #adadb8;";

// Small flat icon button helper
QPushButton *makeIconButton(const QString &text, const QString &tooltip,
                            QWidget *parent, double fontSize = 12)
{
    auto *btn = new QPushButton(text, parent);
    btn->setFlat(true);
    btn->setFixedSize(30, 22);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(
        "QPushButton { background: rgba(255,255,255,0.05); "
        "border: 1px solid rgba(255,255,255,0.18); "
        "border-radius: 4px; color: #dedee3; padding: 0; }"
        "QPushButton:hover { background: rgba(255,255,255,0.18); "
        "border-color: rgba(255,255,255,0.35); }"
        "QPushButton:pressed { background: rgba(255,255,255,0.28); }");
    QFont f = btn->font();
    f.setPointSizeF(fontSize);
    btn->setFont(f);
    if (!tooltip.isEmpty())
    {
        btn->setToolTip(tooltip);
    }
    return btn;
}

}  // namespace

PinnedMessageWidget::PinnedMessageWidget(QWidget *parent)
    : QFrame(parent)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    this->setAutoFillBackground(true);
    this->setBackgroundRole(QPalette::Button);

    auto *outerBox = new QVBoxLayout(this);
    outerBox->setContentsMargins(0, 0, 0, 0);
    outerBox->setSpacing(0);

    // 1px top border — separates from the split header above
    auto *topBorder = new QWidget(this);
    topBorder->setFixedHeight(1);
    topBorder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    topBorder->setStyleSheet(
        QStringLiteral("background-color: palette(shadow);"));
    outerBox->addWidget(topBorder);

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
    this->menuButton_ =
        makeIconButton(QStringLiteral("\u205D"),  // ⁝ tricolon
                       QStringLiteral("Mod options"), this, 10);
    this->menuButton_->hide();
    headerRow->addWidget(this->menuButton_);

    contentBox->addLayout(headerRow);

    // Message body
    this->messageLabel_ = new QTextEdit(this);
    this->messageLabel_->setReadOnly(true);
    this->messageLabel_->setFrameShape(QFrame::NoFrame);
    this->messageLabel_->setStyleSheet("QTextEdit { background: transparent; }");
    this->messageLabel_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->messageLabel_->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    this->messageLabel_->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    this->messageLabel_->setMaximumHeight(110);
    this->messageLabel_->setSizePolicy(QSizePolicy::Expanding,
                                       QSizePolicy::Preferred);
    this->messageLabel_->document()->setDocumentMargin(0);
    this->messageLabel_->setContentsMargins(0, 0, 0, 0);
    {
        QFont f = this->messageLabel_->font();
        f.setPointSize(13);
        this->messageLabel_->setFont(f);
    }
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

    // 1px bottom border — separates pin widget from the chat view below
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

    QObject::connect(this->menuButton_, &QPushButton::clicked, this,
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
            if (pin)
            {
                this->channel_->pinMessage(pin->messageID, seconds);
            }
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

    // deduplicate double @mention that appears when pinning a reply
    QString displayText = pin->messageText;
    {
        static const QRegularExpression doubleAt(
            QStringLiteral("^(@[^\\s]+) \\1(?=\\s|$)"),
            QRegularExpression::CaseInsensitiveOption);
        const auto m = doubleAt.match(displayText);
        if (m.hasMatch())
        {
            displayText.remove(0, m.capturedLength(0) - m.capturedLength(1));
        }
    }
    this->messageLabel_->setPlainText(displayText);

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
    return this->channel_ && this->channel_->getPinnedMessage().has_value();
}

}  // namespace chatterino
