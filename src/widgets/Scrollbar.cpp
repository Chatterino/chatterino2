#include "widgets/Scrollbar.hpp"

#include "common/QLogging.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include <cmath>

namespace {

constexpr int MIN_THUMB_HEIGHT = 10;

/// Amount of messages to move by when clicking on the track
constexpr qreal SCROLL_DELTA = 5.0;

bool areClose(auto a, auto b)
{
    return std::abs(a - b) <= 0.0001;
}

}  // namespace

namespace chatterino {

Scrollbar::Scrollbar(size_t messagesLimit, ChannelView *parent)
    : BaseWidget(parent)
    , currentValueAnimation_(this, "currentValue_")
    , highlights_(messagesLimit)
{
    this->resize(static_cast<int>(16 * this->scale()), 100);
    this->currentValueAnimation_.setDuration(150);
    this->currentValueAnimation_.setEasingCurve(
        QEasingCurve(QEasingCurve::OutCubic));
    connect(&this->currentValueAnimation_, &QAbstractAnimation::finished, this,
            &Scrollbar::resetBounds);

    this->setMouseTracking(true);

    getSettings()->hideScrollbarThumb.connect(
        [this](bool newValue) {
            this->settingHideThumb = newValue;
            this->update();
        },
        this->signalHolder);

    getSettings()->hideScrollbarHighlights.connect(
        [this](bool newValue) {
            this->settingHideHighlights = newValue;
            this->update();
        },
        this->signalHolder);
}

boost::circular_buffer<ScrollbarHighlight> Scrollbar::getHighlights() const
{
    return this->highlights_;
}

void Scrollbar::addHighlight(ScrollbarHighlight highlight)
{
    this->highlights_.push_back(std::move(highlight));
}

void Scrollbar::addHighlightsAtStart(
    const std::vector<ScrollbarHighlight> &highlights)
{
    size_t nItems = std::min(highlights.size(), this->highlights_.capacity() -
                                                    this->highlights_.size());

    if (nItems == 0)
    {
        return;
    }

    for (size_t i = 0; i < nItems; i++)
    {
        this->highlights_.push_front(highlights[highlights.size() - 1 - i]);
    }
}

void Scrollbar::replaceHighlight(size_t index, ScrollbarHighlight replacement)
{
    if (this->highlights_.size() <= index)
    {
        return;
    }

    this->highlights_[index] = std::move(replacement);
}

void Scrollbar::clearHighlights()
{
    this->highlights_.clear();
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

void Scrollbar::resetBounds()
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

void Scrollbar::setPageSize(qreal value)
{
    this->pageSize_ = value;

    this->updateScroll();
}

void Scrollbar::setDesiredValue(qreal value, bool animated)
{
    // this can't use std::clamp, because minimum_ < getBottom() isn't always
    // true, which is a precondition for std::clamp
    value = std::max(this->minimum_, std::min(this->getBottom(), value));
    if (areClose(this->currentValue_, value))
    {
        // value has not changed
        return;
    }

    this->desiredValue_ = value;

    this->desiredValueChanged_.invoke();

    this->atBottom_ = areClose(this->getBottom(), value);

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
            this->resetBounds();
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

qreal Scrollbar::getPageSize() const
{
    return this->pageSize_;
}

qreal Scrollbar::getBottom() const
{
    return this->maximum_ - this->pageSize_;
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
    return std::clamp(this->currentValue_ - this->minimum_, 0.0,
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
    if (areClose(this->currentValue_, value))
    {
        // value has not changed
        return;
    }

    this->currentValue_ = value;

    this->updateScroll();
    this->currentValueChanged_.invoke();
}

void Scrollbar::printCurrentState(const QString &prefix) const
{
    qCDebug(chatterinoWidget).nospace().noquote()
        << prefix                                          //
        << " { currentValue: " << this->getCurrentValue()  //
        << ", desiredValue: " << this->getDesiredValue()   //
        << ", maximum: " << this->getMaximum()             //
        << ", minimum: " << this->getMinimum()             //
        << ", pageSize: " << this->getPageSize()           //
        << " }";
}

void Scrollbar::paintEvent(QPaintEvent * /*event*/)
{
    bool mouseOver = this->mouseOverLocation_ != MouseLocation::Outside;
    int xOffset =
        mouseOver ? 0 : this->width() - static_cast<int>(4.0F * this->scale());

    QPainter painter(this);
    painter.fillRect(this->rect(), this->theme->scrollbars.background);

    bool enableRedeemedHighlights = getSettings()->enableRedeemedHighlight;
    bool enableFirstMessageHighlights =
        getSettings()->enableFirstMessageHighlight;
    bool enableElevatedMessageHighlights =
        getSettings()->enableElevatedMessageHighlight;

    if (this->shouldShowThumb())
    {
        this->thumbRect_.setX(xOffset);

        // mouse over thumb
        if (this->mouseDownLocation_ == MouseLocation::InsideThumb)
        {
            painter.fillRect(this->thumbRect_,
                             this->theme->scrollbars.thumbSelected);
        }
        // mouse not over thumb
        else
        {
            painter.fillRect(this->thumbRect_, this->theme->scrollbars.thumb);
        }
    }

    if (this->shouldShowHighlights() && !this->highlights_.empty())
    {
        size_t nHighlights = this->highlights_.size();
        int w = this->width();
        float dY = static_cast<float>(this->height()) /
                   static_cast<float>(nHighlights);
        int highlightHeight =
            static_cast<int>(std::ceil(std::max(this->scale() * 2.0F, dY)));

        for (size_t i = 0; i < nHighlights; i++)
        {
            const auto &highlight = this->highlights_[i];

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

            int y = static_cast<int>(dY * static_cast<float>(i));
            switch (highlight.getStyle())
            {
                case ScrollbarHighlight::Default: {
                    painter.fillRect(w / 8 * 3, y, w / 4, highlightHeight,
                                     color);
                }
                break;

                case ScrollbarHighlight::Line: {
                    painter.fillRect(0, y, w, 1, color);
                }
                break;

                case ScrollbarHighlight::None:;
            }
        }
    }
}

void Scrollbar::resizeEvent(QResizeEvent * /*event*/)
{
    this->resize(static_cast<int>(16 * this->scale()), this->height());
}

void Scrollbar::mouseMoveEvent(QMouseEvent *event)
{
    if (!this->shouldHandleMouseEvents())
    {
        return;
    }

    if (this->mouseDownLocation_ == MouseLocation::Outside)
    {
        auto moveLocation = this->locationOfMouseEvent(event);
        if (this->mouseOverLocation_ != moveLocation)
        {
            this->mouseOverLocation_ = moveLocation;
            this->update();
        }
    }
    else if (this->mouseDownLocation_ == MouseLocation::InsideThumb)
    {
        qreal delta =
            static_cast<qreal>(event->pos().y() - this->lastMousePosition_.y());

        this->setDesiredValue(
            this->desiredValue_ +
            (delta / std::max<qreal>(0.00000002, this->trackHeight_)) *
                this->maximum_);
    }

    this->lastMousePosition_ = event->pos();
}

void Scrollbar::mousePressEvent(QMouseEvent *event)
{
    if (!this->shouldHandleMouseEvents())
    {
        return;
    }

    this->mouseDownLocation_ = this->locationOfMouseEvent(event);
    this->update();
}

void Scrollbar::mouseReleaseEvent(QMouseEvent *event)
{
    if (!this->shouldHandleMouseEvents())
    {
        return;
    }

    auto releaseLocation = this->locationOfMouseEvent(event);
    if (this->mouseDownLocation_ != releaseLocation)
    {
        // Ignore event. User released the mouse from a different spot than
        // they first clicked. For example, they clicked above the thumb,
        // changed their mind, dragged the mouse below the thumb, and released.
        this->mouseDownLocation_ = MouseLocation::Outside;
        return;
    }

    switch (releaseLocation)
    {
        case MouseLocation::AboveThumb:
            // Move scrollbar up a small bit.
            this->setDesiredValue(this->desiredValue_ - SCROLL_DELTA, true);
            break;
        case MouseLocation::BelowThumb:
            // Move scrollbar down a small bit.
            this->setDesiredValue(this->desiredValue_ + SCROLL_DELTA, true);
            break;
        default:
            break;
    }

    this->mouseDownLocation_ = MouseLocation::Outside;
    this->update();
}

void Scrollbar::leaveEvent(QEvent * /*event*/)
{
    this->mouseOverLocation_ = MouseLocation::Outside;
    this->update();
}

void Scrollbar::updateScroll()
{
    this->trackHeight_ = this->height() - MIN_THUMB_HEIGHT - 1;

    auto div = std::max<qreal>(0.0000001, this->maximum_ - this->minimum_);

    this->thumbRect_ =
        QRect(0,
              static_cast<int>((this->getRelativeCurrentValue()) / div *
                               this->trackHeight_) +
                  1,
              this->width(),
              static_cast<int>(this->pageSize_ / div * this->trackHeight_) +
                  MIN_THUMB_HEIGHT);

    this->update();
}

void Scrollbar::setHideThumb(bool hideThumb)
{
    if (this->hideThumb == hideThumb)
    {
        return;
    }

    this->hideThumb = hideThumb;
    this->update();
}

bool Scrollbar::shouldShowThumb() const
{
    return !(this->hideThumb || this->settingHideThumb);
}

void Scrollbar::setHideHighlights(bool hideHighlights)
{
    if (this->hideHighlights == hideHighlights)
    {
        return;
    }

    this->hideHighlights = hideHighlights;
    this->update();
}

bool Scrollbar::shouldShowHighlights() const
{
    return !(this->hideHighlights || this->settingHideHighlights);
}

bool Scrollbar::shouldHandleMouseEvents() const
{
    return this->shouldShowThumb();
}

Scrollbar::MouseLocation Scrollbar::locationOfMouseEvent(
    QMouseEvent *event) const
{
    int y = event->pos().y();

    if (y < this->thumbRect_.y())
    {
        return MouseLocation::AboveThumb;
    }

    if (this->thumbRect_.contains(2, y))
    {
        return MouseLocation::InsideThumb;
    }

    return MouseLocation::BelowThumb;
}

}  // namespace chatterino
