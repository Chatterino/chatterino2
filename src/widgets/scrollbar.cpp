#include "widgets/scrollbar.hpp"
#include "singletons/thememanager.hpp"
#include "widgets/helper/channelview.hpp"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <cmath>

#define MIN_THUMB_HEIGHT 10

namespace chatterino {
namespace widgets {

ScrollBar::ScrollBar(ChannelView *parent)
    : BaseWidget(parent)
    , currentValueAnimation(this, "currentValue")
    , highlights(nullptr)
    , smoothScrollingSetting(SettingsManager::getInstance().enableSmoothScrolling)
{
    resize((int)(16 * this->getDpiMultiplier()), 100);
    this->currentValueAnimation.setDuration(250);
    this->currentValueAnimation.setEasingCurve(QEasingCurve(QEasingCurve::OutCubic));

    setMouseTracking(true);

    // don't do this at home kids
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);

    connect(timer, &QTimer::timeout, [=]() {
        resize((int)(16 * this->getDpiMultiplier()), 100);
        timer->deleteLater();
    });

    timer->start(10);
}

ScrollBar::~ScrollBar()
{
    auto highlight = this->highlights;

    while (highlight != nullptr) {
        auto tmp = highlight->next;
        delete highlight;
        highlight = tmp;
    }
}

void ScrollBar::removeHighlightsWhere(std::function<bool(ScrollBarHighlight &)> func)
{
    this->mutex.lock();

    ScrollBarHighlight *last = nullptr;
    ScrollBarHighlight *current = this->highlights;

    while (current != nullptr) {
        if (func(*current)) {
            if (last == nullptr) {
                this->highlights = current->next;
            } else {
                last->next = current->next;
            }

            auto oldCurrent = current;

            current = current->next;
            last = current;

            delete oldCurrent;
        }
    }

    this->mutex.unlock();
}

void ScrollBar::addHighlight(ScrollBarHighlight *highlight)
{
    this->mutex.lock();

    if (this->highlights == nullptr) {
        this->highlights = highlight;
    } else {
        highlight->next = this->highlights->next;
        this->highlights->next = highlight;
    }

    this->mutex.unlock();
}

void ScrollBar::scrollToBottom()
{
    this->setDesiredValue(this->maximum - this->getLargeChange());
}

bool ScrollBar::isAtBottom() const
{
    return this->atBottom;
}

void ScrollBar::setMaximum(qreal value)
{
    this->maximum = value;

    updateScroll();
}

void ScrollBar::setMinimum(qreal value)
{
    this->minimum = value;

    updateScroll();
}

void ScrollBar::setLargeChange(qreal value)
{
    this->largeChange = value;

    updateScroll();
}

void ScrollBar::setSmallChange(qreal value)
{
    this->smallChange = value;

    updateScroll();
}

void ScrollBar::setDesiredValue(qreal value, bool animated)
{
    animated &= this->smoothScrollingSetting.getValue();
    value = std::max(this->minimum, std::min(this->maximum - this->largeChange, value));

    if (this->desiredValue + this->smoothScrollingOffset != value) {
        if (animated) {
            this->currentValueAnimation.stop();
            this->currentValueAnimation.setStartValue(this->currentValue +
                                                      this->smoothScrollingOffset);

            //            if (((this->getMaximum() - this->getLargeChange()) - value) <= 0.01) {
            //                value += 1;
            //            }
            this->currentValueAnimation.setEndValue(value);
            this->smoothScrollingOffset = 0;
            this->atBottom = ((this->getMaximum() - this->getLargeChange()) - value) <= 0.01;
            this->currentValueAnimation.start();
        } else {
            if (this->currentValueAnimation.state() != QPropertyAnimation::Running) {
                this->smoothScrollingOffset = 0;
                this->desiredValue = value;
                this->currentValueAnimation.stop();
                this->atBottom = ((this->getMaximum() - this->getLargeChange()) - value) <= 0.01;
                setCurrentValue(value);
            }
        }
    }

    this->smoothScrollingOffset = 0;
    this->desiredValue = value;
}

qreal ScrollBar::getMaximum() const
{
    return this->maximum;
}

qreal ScrollBar::getMinimum() const
{
    return this->minimum;
}

qreal ScrollBar::getLargeChange() const
{
    return this->largeChange;
}

qreal ScrollBar::getSmallChange() const
{
    return this->smallChange;
}

qreal ScrollBar::getDesiredValue() const
{
    return this->desiredValue + this->smoothScrollingOffset;
}

qreal ScrollBar::getCurrentValue() const
{
    return this->currentValue;
}

void ScrollBar::offset(qreal value)
{
    if (this->currentValueAnimation.state() == QPropertyAnimation::Running) {
        this->smoothScrollingOffset += value;
    } else {
        this->setDesiredValue(this->getDesiredValue() + value);
    }
}

boost::signals2::signal<void()> &ScrollBar::getCurrentValueChanged()
{
    return this->currentValueChanged;
}

void ScrollBar::setCurrentValue(qreal value)
{
    value = std::max(this->minimum, std::min(this->maximum - this->largeChange,
                                             value + this->smoothScrollingOffset));

    if (std::abs(this->currentValue - value) > 0.000001) {
        this->currentValue = value;

        this->updateScroll();
        this->currentValueChanged();

        this->update();
    }
}

void ScrollBar::printCurrentState(const QString &prefix) const
{
    qDebug() << prefix                                         //
             << "Current value: " << this->getCurrentValue()   //
             << ". Maximum: " << this->getMaximum()            //
             << ". Minimum: " << this->getMinimum()            //
             << ". Large change: " << this->getLargeChange();  //
}

void ScrollBar::paintEvent(QPaintEvent *)
{
    bool mouseOver = this->mouseOverIndex != -1;
    int xOffset = mouseOver ? 0 : width() - (int)(4 * this->getDpiMultiplier());

    QPainter painter(this);
    //    painter.fillRect(rect(), this->themeManager.ScrollbarBG);

    painter.fillRect(QRect(xOffset, 0, width(), this->buttonHeight),
                     this->themeManager.ScrollbarArrow);
    painter.fillRect(QRect(xOffset, height() - this->buttonHeight, width(), this->buttonHeight),
                     this->themeManager.ScrollbarArrow);

    this->thumbRect.setX(xOffset);

    // mouse over thumb
    if (this->mouseDownIndex == 2) {
        painter.fillRect(this->thumbRect, this->themeManager.ScrollbarThumbSelected);
    }
    // mouse not over thumb
    else {
        painter.fillRect(this->thumbRect, this->themeManager.ScrollbarThumb);
    }

    //    ScrollBarHighlight *highlight = highlights;

    this->mutex.lock();

    //    do {
    //        painter.fillRect();
    //    } while ((highlight = highlight->next()) != nullptr);

    this->mutex.unlock();
}

void ScrollBar::resizeEvent(QResizeEvent *)
{
    this->resize((int)(16 * this->getDpiMultiplier()), this->height());
}

void ScrollBar::mouseMoveEvent(QMouseEvent *event)
{
    if (this->mouseDownIndex == -1) {
        int y = event->pos().y();

        auto oldIndex = this->mouseOverIndex;

        if (y < this->buttonHeight) {
            this->mouseOverIndex = 0;
        } else if (y < this->thumbRect.y()) {
            this->mouseOverIndex = 1;
        } else if (this->thumbRect.contains(2, y)) {
            this->mouseOverIndex = 2;
        } else if (y < height() - this->buttonHeight) {
            this->mouseOverIndex = 3;
        } else {
            this->mouseOverIndex = 4;
        }

        if (oldIndex != this->mouseOverIndex) {
            update();
        }
    } else if (this->mouseDownIndex == 2) {
        int delta = event->pos().y() - this->lastMousePosition.y();

        setDesiredValue(this->desiredValue + (qreal)delta / this->trackHeight * this->maximum);
    }

    this->lastMousePosition = event->pos();
}

void ScrollBar::mousePressEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < this->buttonHeight) {
        this->mouseDownIndex = 0;
    } else if (y < this->thumbRect.y()) {
        this->mouseDownIndex = 1;
    } else if (this->thumbRect.contains(2, y)) {
        this->mouseDownIndex = 2;
    } else if (y < height() - this->buttonHeight) {
        this->mouseDownIndex = 3;
    } else {
        this->mouseDownIndex = 4;
    }
}

void ScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < this->buttonHeight) {
        if (this->mouseDownIndex == 0) {
            setDesiredValue(this->desiredValue - this->smallChange, true);
        }
    } else if (y < this->thumbRect.y()) {
        if (this->mouseDownIndex == 1) {
            setDesiredValue(this->desiredValue - this->smallChange, true);
        }
    } else if (this->thumbRect.contains(2, y)) {
        // do nothing
    } else if (y < height() - this->buttonHeight) {
        if (this->mouseDownIndex == 3) {
            setDesiredValue(this->desiredValue + this->smallChange, true);
        }
    } else {
        if (this->mouseDownIndex == 4) {
            setDesiredValue(this->desiredValue + this->smallChange, true);
        }
    }

    this->mouseDownIndex = -1;
    update();
}

void ScrollBar::leaveEvent(QEvent *)
{
    this->mouseOverIndex = -1;

    update();
}

void ScrollBar::updateScroll()
{
    this->trackHeight = height() - this->buttonHeight - this->buttonHeight - MIN_THUMB_HEIGHT - 1;

    this->thumbRect = QRect(
        0, (int)(this->currentValue / this->maximum * this->trackHeight) + 1 + this->buttonHeight,
        width(), (int)(this->largeChange / this->maximum * this->trackHeight) + MIN_THUMB_HEIGHT);

    update();
}

}  // namespace widgets
}  // namespace chatterino
