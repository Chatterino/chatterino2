#include "widgets/Scrollbar.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

#include <cmath>

#define MIN_THUMB_HEIGHT 10

namespace chatterino {

Scrollbar::Scrollbar(ChannelView *parent)
    : BaseWidget(parent)
    , currentValueAnimation_(this, "currentValue_")
{
    resize(int(16 * this->scale()), 100);
    this->currentValueAnimation_.setDuration(150);
    this->currentValueAnimation_.setEasingCurve(
        QEasingCurve(QEasingCurve::OutCubic));

    setMouseTracking(true);
}

void Scrollbar::addHighlight(ScrollbarHighlight highlight)
{
    ScrollbarHighlight deleted;
    this->highlights_.pushBack(highlight, deleted);
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

LimitedQueueSnapshot<ScrollbarHighlight> Scrollbar::getHighlightSnapshot()
{
    if (!this->highlightsPaused_)
    {
        this->highlightSnapshot_ = this->highlights_.getSnapshot();
    }

    return this->highlightSnapshot_;
}

void Scrollbar::scrollToBottom(bool animate)
{
    this->setDesiredValue(this->maximum_ - this->getLargeChange(), animate);
}

bool Scrollbar::isAtBottom() const
{
    return this->atBottom_;
}

void Scrollbar::setMaximum(qreal value)
{
    this->maximum_ = value;

    updateScroll();
}

void Scrollbar::setMinimum(qreal value)
{
    this->minimum_ = value;

    updateScroll();
}

void Scrollbar::setLargeChange(qreal value)
{
    this->largeChange_ = value;

    updateScroll();
}

void Scrollbar::setSmallChange(qreal value)
{
    this->smallChange_ = value;

    updateScroll();
}

void Scrollbar::setDesiredValue(qreal value, bool animated)
{
    animated &= getSettings()->enableSmoothScrolling;
    value = std::max(this->minimum_,
                     std::min(this->maximum_ - this->largeChange_, value));

    if (std::abs(this->desiredValue_ + this->smoothScrollingOffset_ - value) >
        0.0001)
    {
        if (animated)
        {
            this->currentValueAnimation_.stop();
            this->currentValueAnimation_.setStartValue(
                this->currentValue_ + this->smoothScrollingOffset_);

            //            if (((this->getMaximum() - this->getLargeChange()) -
            //            value) <= 0.01) {
            //                value += 1;
            //            }

            this->currentValueAnimation_.setEndValue(value);
            this->smoothScrollingOffset_ = 0;
            this->atBottom_ = ((this->getMaximum() - this->getLargeChange()) -
                               value) <= 0.0001;
            this->currentValueAnimation_.start();
        }
        else
        {
            if (this->currentValueAnimation_.state() !=
                QPropertyAnimation::Running)
            {
                this->smoothScrollingOffset_ = 0;
                this->desiredValue_ = value;
                this->currentValueAnimation_.stop();
                this->atBottom_ =
                    ((this->getMaximum() - this->getLargeChange()) - value) <=
                    0.0001;
                setCurrentValue(value);
            }
        }
    }

    this->smoothScrollingOffset_ = 0;
    this->desiredValue_ = value;

    this->desiredValueChanged_.invoke();
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

qreal Scrollbar::getSmallChange() const
{
    return this->smallChange_;
}

qreal Scrollbar::getDesiredValue() const
{
    return this->desiredValue_ + this->smoothScrollingOffset_;
}

qreal Scrollbar::getCurrentValue() const
{
    return this->currentValue_;
}

void Scrollbar::offset(qreal value)
{
    if (this->currentValueAnimation_.state() == QPropertyAnimation::Running)
    {
        this->smoothScrollingOffset_ += value;
    }
    else
    {
        this->setDesiredValue(this->getDesiredValue() + value);
    }
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
    value = std::max(this->minimum_,
                     std::min(this->maximum_ - this->largeChange_,
                              value + this->smoothScrollingOffset_));

    if (std::abs(this->currentValue_ - value) > 0.0001)
    {
        this->currentValue_ = value;

        this->updateScroll();
        this->currentValueChanged_.invoke();

        this->update();
    }
}

void Scrollbar::printCurrentState(const QString &prefix) const
{
    qDebug() << prefix                                         //
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
    auto snapshot = this->getHighlightSnapshot();
    size_t snapshotLength = snapshot.size();

    if (snapshotLength == 0)
    {
        return;
    }

    int w = this->width();
    float y = 0;
    float dY = float(this->height()) / float(snapshotLength);
    int highlightHeight = int(std::ceil(dY));

    for (size_t i = 0; i < snapshotLength; i++)
    {
        ScrollbarHighlight const &highlight = snapshot[i];

        if (!highlight.isNull())
        {
            QColor color = [&] {
                switch (highlight.getType())
                {
                    case ScrollbarHighlight::Type::Subscription:
                        return getApp()
                            ->themes->scrollbars.highlights.subscription;
                    case ScrollbarHighlight::Type::Highlight:
                        return highlight.getColor();
                }
                return QColor();
            }();

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

        y += dY;
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
            update();
        }
    }
    else if (this->mouseDownIndex_ == 2)
    {
        int delta = event->pos().y() - this->lastMousePosition_.y();

        setDesiredValue(this->desiredValue_ +
                        qreal(delta) / this->trackHeight_ * this->maximum_);
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
            setDesiredValue(this->desiredValue_ - this->smallChange_, true);
        }
    }
    else if (y < this->thumbRect_.y())
    {
        if (this->mouseDownIndex_ == 1)
        {
            setDesiredValue(this->desiredValue_ - this->smallChange_, true);
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
            setDesiredValue(this->desiredValue_ + this->smallChange_, true);
        }
    }
    else
    {
        if (this->mouseDownIndex_ == 4)
        {
            setDesiredValue(this->desiredValue_ + this->smallChange_, true);
        }
    }

    this->mouseDownIndex_ = -1;
    update();
}

void Scrollbar::leaveEvent(QEvent *)
{
    this->mouseOverIndex_ = -1;

    update();
}

void Scrollbar::updateScroll()
{
    this->trackHeight_ = this->height() - this->buttonHeight_ -
                         this->buttonHeight_ - MIN_THUMB_HEIGHT - 1;

    this->thumbRect_ =
        QRect(0,
              int(this->currentValue_ / this->maximum_ * this->trackHeight_) +
                  1 + this->buttonHeight_,
              this->width(),
              int(this->largeChange_ / this->maximum_ * this->trackHeight_) +
                  MIN_THUMB_HEIGHT);

    this->update();
}

}  // namespace chatterino
