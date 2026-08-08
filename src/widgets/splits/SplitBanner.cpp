// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/splits/SplitBanner.hpp"

#include "singletons/Theme.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QVBoxLayout>

using namespace std::chrono_literals;
using namespace Qt::Literals;

namespace chatterino {

SplitBanner::SplitBanner(QWidget *parent)
    : BaseWidget(parent)
    , headerLabel_(new QLabel(this))
    , countdownLabel_(new QLabel(this))
    , headerRow_(new QHBoxLayout())
    , contentBox_(new QVBoxLayout())
    , countdownTimer_(new QTimer(this))
    , autoHideTimer_(new QTimer(this))
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto *outerBox = new QVBoxLayout(this);
    outerBox->setContentsMargins(0, 0, 0, 0);
    outerBox->setSpacing(0);

    this->contentBox_->setContentsMargins(8, 6, 8, 6);
    this->contentBox_->setSpacing(3);

    this->headerRow_->setSpacing(4);
    this->headerRow_->addWidget(this->headerLabel_);
    this->headerRow_->addStretch(1);
    this->contentBox_->addLayout(this->headerRow_);

    outerBox->addLayout(this->contentBox_);

    // 1px bottom border - separates the banner from the chat view below
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

    this->countdownLabel_->setStyleSheet(MUTED_STYLE);
    this->countdownLabel_->hide();

    this->countdownTimer_->setInterval(1s);
    QObject::connect(this->countdownTimer_, &QTimer::timeout, this, [this] {
        this->tickCountdown();
    });

    this->autoHideTimer_->setSingleShot(true);
    QObject::connect(this->autoHideTimer_, &QTimer::timeout, this, [this] {
        this->autoHide();
    });
}

QLabel *SplitBanner::headerLabel() const
{
    return this->headerLabel_;
}

QHBoxLayout *SplitBanner::headerRow() const
{
    return this->headerRow_;
}

QVBoxLayout *SplitBanner::contentBox() const
{
    return this->contentBox_;
}

QLabel *SplitBanner::countdownLabel() const
{
    return this->countdownLabel_;
}

void SplitBanner::startCountdown()
{
    this->tickCountdown();
    this->countdownTimer_->start();
}

void SplitBanner::stopCountdown()
{
    this->countdownTimer_->stop();
}

void SplitBanner::startAutoHide(std::chrono::milliseconds delay)
{
    this->autoHideTimer_->start(delay);
}

void SplitBanner::stopAutoHide()
{
    this->autoHideTimer_->stop();
}

void SplitBanner::tickCountdown()
{
}

void SplitBanner::autoHide()
{
    this->hide();
}

QString SplitBanner::formatCountdown(qint64 millis)
{
    const qint64 totalSecs = (millis + 999) / 1000;  // round up
    const qint64 hours = totalSecs / 3600;
    const qint64 mins = (totalSecs % 3600) / 60;
    const qint64 secs = totalSecs % 60;

    if (hours > 0)
    {
        return u"%1:%2:%3"_s.arg(hours)
            .arg(mins, 2, 10, QChar(u'0'))
            .arg(secs, 2, 10, QChar(u'0'));
    }

    return u"%1:%2"_s.arg(mins, 2, 10, QChar(u'0'))
        .arg(secs, 2, 10, QChar(u'0'));
}

void SplitBanner::scaleChangedEvent(float newScale)
{
    QFont headerFont = this->headerLabel_->font();
    headerFont.setPointSizeF(9.5F * newScale);
    this->headerLabel_->setFont(headerFont);
    this->countdownLabel_->setFont(headerFont);
}

void SplitBanner::mousePressEvent(QMouseEvent *event)
{
    // ignore to disable the parent's right click menu
}

void SplitBanner::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    auto *theme = getTheme();

    // Fill background (same color as the split header above)
    painter.fillRect(event->rect(), theme->splits.header.background);

    // Draw 1px top border
    painter.setPen(theme->splits.header.border);
    painter.drawLine(0, 0, this->width() - 1, 0);
}

}  // namespace chatterino
