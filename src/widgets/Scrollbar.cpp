#include "widgets/Scrollbar.hpp"

#include "common/QLogging.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/Clamp.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include <cmath>

#define MIN_THUMB_HEIGHT 10

namespace chatterino {

Scrollbar::Scrollbar(size_t messagesLimit, ChannelView *parent)
    : BaseWidget(parent)
    , currentValueAnimation_(this, "currentValue_")
    , highlights_(messagesLimit)
{
    this->resize(int(16 * this->scale()), 100);
    this->currentValueAnimation_.setDuration(150);
    this->currentValueAnimation_.setEasingCurve(
        QEasingCurve(QEasingCurve::OutCubic));
    connect(&this->currentValueAnimation_, &QAbstractAnimation::finished, this,
            &Scrollbar::resetMaximum);

    this->setMouseTracking(true);
}

void Scrollbar::addHighlight(ScrollbarHighlight highlight)
{
    this->highlights_.pushBack(highlight);
}

void Scrollbar::addHighlightsAtStart(
    const std::vector<ScrollbarHighlight> &_highlights)
{
    this->highlights_.pushFront(_highlights);
}

void Scrollbar::replaceHighlight(size_t index, ScrollbarHighlight replacement)
{
    this->highlights_.replaceItem(index, replacement);
}

void Scrollbar::pauseHighlights()
{
    this->highlightsPaused_ = true;
}

void Scrollbar::unpauseHighlights()
{
    this->highlightsPaused_ = false;
}

void Scrollbar::clearHighlights()
{
    this->highlights_.clear();
}

LimitedQueueSnapshot<ScrollbarHighlight> &Scrollbar::getHighlightSnapshot()
{
    if (!this->highlightsPaused_)
    {
        this->highlightSnapshot_ = this->highlights_.getSnapshot();
    }

    return this->highlightSnapshot_;
}

void Scrollbar::scrollToBottom(bool animate)
{
    this->setDesiredValue(this->getBottom(), animate);
}

void Scrollbar::scrollToTop(bool animate)
{
    this->setDesiredValue(this->minimum_, animate);
}

bool Scrollbar::isAtBottom() const
{
    return this->atBottom_;
}

void Scrollbar::setMaximum(qreal value)
{
    this->maximum_ = value;

    this->updateScroll();
}

void Scrollbar::offsetMaximum(qreal value)
{
    this->maximum_ += value;

    this->updateScroll();
}

void Scrollbar::resetMaximum()
{
    if (this->minimum_ > 0)
    {
        this->maximum_ -= this->minimum_;
        this->desiredValue_ -= this->minimum_;
        this->currentValue_ -= this->minimum_;
        this->minimum_ = 0;
    }
}

void Scrollbar::setMinimum(qreal value)
{
    this->minimum_ = value;

    this->updateScroll();
}

void Scrollbar::offsetMinimum(qreal value)
{
    this->minimum_ += value;

    if (this->minimum_ > this->desiredValue_)
    {
        this->scrollToTop();
    }

    this->updateScroll();
}

void Scrollbar::setLargeChange(qreal value)
{
    this->largeChange_ = value;

    this->updateScroll();
}

void Scrollbar::setSmallChange(qreal value)
{
    this->smallChange_ = value;

    this->updateScroll();
}

void Scrollbar::setDesiredValue(qreal value, bool animated)
{
    value = std::max(this->minimum_, std::min(this->getBottom(), value));

    if (std::abs(this->currentValue_ - value) <= 0.0001)
    {
        return;
    }

    this->desiredValue_ = value;

    this->desiredValueChanged_.invoke();

    this->atBottom_ = (this->getBottom() - value) <= 0.0001;

    if (animated && getSettings()->enableSmoothScrolling)
    {
        // stop() does not emit QAbstractAnimation::finished().
        this->currentValueAnimation_.stop();
        this->currentValueAnimation_.setStartValue(this->currentValue_);
        this->currentValueAnimation_.setEndValue(value);
        this->currentValueAnimation_.start();
    }
    else
    {
        if (this->currentValueAnimation_.state() != QPropertyAnimation::Stopped)
        {
            this->currentValueAnimation_.setEndValue(value);
        }
        else
        {
            this->setCurrentValue(value);
            this->resetMaximum();
        }
    }
}

qreal Scrollbar::getMaximum() const
{
    return this->maximum_;
}

qreal Scrollbar::getMinimum() const
{
    return this->minimum_;
}

qreal Scrollbar::getLargeChange() const
{
    return this->largeChange_;
}

qreal Scrollbar::getBottom() const
{
    return this->maximum_ - this->largeChange_;
}

qreal Scrollbar::getSmallChange() const
{
    return this->smallChange_;
}

qreal Scrollbar::getDesiredValue() const
{
    return this->desiredValue_;
}

qreal Scrollbar::getCurrentValue() const
{
    return this->currentValue_;
}

qreal Scrollbar::getRelativeCurrentValue() const
{
    // currentValue - minimum can be negative if minimum is incremented while
    // scrolling up to or down from the top when smooth scrolling is enabled.
    return clamp(this->currentValue_ - this->minimum_, qreal(0.0),
                 this->currentValue_);
}

void Scrollbar::offset(qreal value)
{
    this->setDesiredValue(this->desiredValue_ + value);
}

pajlada::Signals::NoArgSignal &Scrollbar::getCurrentValueChanged()
{
    return this->currentValueChanged_;
}

pajlada::Signals::NoArgSignal &Scrollbar::getDesiredValueChanged()
{
    return this->desiredValueChanged_;
}

void Scrollbar::setCurrentValue(qreal value)
{
    value = std::max(this->minimum_, std::min(this->getBottom(), value));

    if (std::abs(this->currentValue_ - value) <= 0.0001)
    {
        return;
    }

    this->currentValue_ = value;

    this->updateScroll();
    this->currentValueChanged_.invoke();
}

void Scrollbar::printCurrentState(const QString &prefix) const
{
    qCDebug(chatterinoWidget)
        << prefix                                         //
        << "Current value: " << this->getCurrentValue()   //
        << ". Maximum: " << this->getMaximum()            //
        << ". Minimum: " << this->getMinimum()            //
        << ". Large change: " << this->getLargeChange();  //
}

void Scrollbar::paintEvent(QPaintEvent *)
{
    bool mouseOver = this->mouseOverIndex_ != -1;
    int xOffset = mouseOver ? 0 : width() - int(4 * this->scale());

    QPainter painter(this);
    painter.fillRect(rect(), this->theme->scrollbars.background);

    bool enableRedeemedHighlights = getSettings()->enableRedeemedHighlight;
    bool enableFirstMessageHighlights =
        getSettings()->enableFirstMessageHighlight;
    bool enableElevatedMessageHighlights =
        getSettings()->enableElevatedMessageHighlight;

    //    painter.fillRect(QRect(xOffset, 0, width(), this->buttonHeight),
    //                     this->themeManager->ScrollbarArrow);
    //    painter.fillRect(QRect(xOffset, height() - this->buttonHeight,
    //    width(), this->buttonHeight),
    //                     this->themeManager->ScrollbarArrow);

    this->thumbRect_.setX(xOffset);

    // mouse over thumb
    if (this->mouseDownIndex_ == 2)
    {
        painter.fillRect(this->thumbRect_,
                         this->theme->scrollbars.thumbSelected);
    }
    // mouse not over thumb
    else
    {
        painter.fillRect(this->thumbRect_, this->theme->scrollbars.thumb);
    }

    // draw highlights
    auto &snapshot = this->getHighlightSnapshot();
    size_t snapshotLength = snapshot.size();

    if (snapshotLength == 0)
    {
        return;
    }

    int w = this->width();
    float y = 0;
    float dY = float(this->height()) / float(snapshotLength);
    int highlightHeight =
        int(std::ceil(std::max<float>(this->scale() * 2, dY)));

    for (size_t i = 0; i < snapshotLength; i++, y += dY)
    {
        ScrollbarHighlight const &highlight = snapshot[i];

        if (highlight.isNull())
        {
            continue;
        }

        if (highlight.isRedeemedHighlight() && !enableRedeemedHighlights)
        {
            continue;
        }

        if (highlight.isFirstMessageHighlight() &&
            !enableFirstMessageHighlights)
        {
            continue;
        }

        if (highlight.isElevatedMessageHighlight() &&
            !enableElevatedMessageHighlights)
        {
            continue;
        }

        QColor color = highlight.getColor();
        color.setAlpha(255);

        switch (highlight.getStyle())
        {
            case ScrollbarHighlight::Default: {
                painter.fillRect(w / 8 * 3, int(y), w / 4, highlightHeight,
                                 color);
            }
            break;

            case ScrollbarHighlight::Line: {
                painter.fillRect(0, int(y), w, 1, color);
            }
            break;

            case ScrollbarHighlight::None:;
        }
    }
}

void Scrollbar::resizeEvent(QResizeEvent *)
{
    this->resize(int(16 * this->scale()), this->height());
}

void Scrollbar::mouseMoveEvent(QMouseEvent *event)
{
    if (this->mouseDownIndex_ == -1)
    {
        int y = event->pos().y();

        auto oldIndex = this->mouseOverIndex_;

        if (y < this->buttonHeight_)
        {
            this->mouseOverIndex_ = 0;
        }
        else if (y < this->thumbRect_.y())
        {
            this->mouseOverIndex_ = 1;
        }
        else if (this->thumbRect_.contains(2, y))
        {
            this->mouseOverIndex_ = 2;
        }
        else if (y < height() - this->buttonHeight_)
        {
            this->mouseOverIndex_ = 3;
        }
        else
        {
            this->mouseOverIndex_ = 4;
        }

        if (oldIndex != this->mouseOverIndex_)
        {
            this->update();
        }
    }
    else if (this->mouseDownIndex_ == 2)
    {
        int delta = event->pos().y() - this->lastMousePosition_.y();

        this->setDesiredValue(
            this->desiredValue_ +
            (qreal(delta) / std::max<qreal>(0.00000002, this->trackHeight_)) *
                this->maximum_);
    }

    this->lastMousePosition_ = event->pos();
}

void Scrollbar::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < this->buttonHeight_)
    {
        this->mouseDownIndex_ = 0;
    }
    else if (y < this->thumbRect_.y())
    {
        this->mouseDownIndex_ = 1;
    }
    else if (this->thumbRect_.contains(2, y))
    {
        this->mouseDownIndex_ = 2;
    }
    else if (y < height() - this->buttonHeight_)
    {
        this->mouseDownIndex_ = 3;
    }
    else
    {
        this->mouseDownIndex_ = 4;
    }
}

void Scrollbar::mouseReleaseEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < this->buttonHeight_)
    {
        if (this->mouseDownIndex_ == 0)
        {
            this->setDesiredValue(this->desiredValue_ - this->smallChange_,
                                  true);
        }
    }
    else if (y < this->thumbRect_.y())
    {
        if (this->mouseDownIndex_ == 1)
        {
            this->setDesiredValue(this->desiredValue_ - this->smallChange_,
                                  true);
        }
    }
    else if (this->thumbRect_.contains(2, y))
    {
        // do nothing
    }
    else if (y < height() - this->buttonHeight_)
    {
        if (this->mouseDownIndex_ == 3)
        {
            this->setDesiredValue(this->desiredValue_ + this->smallChange_,
                                  true);
        }
    }
    else
    {
        if (this->mouseDownIndex_ == 4)
        {
            this->setDesiredValue(this->desiredValue_ + this->smallChange_,
                                  true);
        }
    }

    this->mouseDownIndex_ = -1;

    this->update();
}

void Scrollbar::leaveEvent(QEvent *)
{
    this->mouseOverIndex_ = -1;

    this->update();
}

void Scrollbar::updateScroll()
{
    this->trackHeight_ = this->height() - this->buttonHeight_ -
                         this->buttonHeight_ - MIN_THUMB_HEIGHT - 1;

    auto div = std::max<qreal>(0.0000001, this->maximum_ - this->minimum_);

    this->thumbRect_ = QRect(
        0,
        int((this->getRelativeCurrentValue()) / div * this->trackHeight_) + 1 +
            this->buttonHeight_,
        this->width(),
        int(this->largeChange_ / div * this->trackHeight_) + MIN_THUMB_HEIGHT);

    this->update();
}

}  // namespace chatterino
