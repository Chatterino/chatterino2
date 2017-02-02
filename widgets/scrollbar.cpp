#include "widgets/scrollbar.h"
#include "colorscheme.h"

#include <QMouseEvent>
#include <QPainter>

#define MIN_THUMB_HEIGHT 10

namespace chatterino {
namespace widgets {

ScrollBar::ScrollBar(QWidget *widget)
    : QWidget(widget)
    , mutex()
    , currentValueAnimation(this, "currentValue")
    , highlights(NULL)
    , mouseOverIndex(-1)
    , mouseDownIndex(-1)
    , lastMousePosition()
    , buttonHeight(16)
    , trackHeight(100)
    , thumbRect()
    , maximum()
    , minimum()
    , largeChange()
    , smallChange()
    , desiredValue()
    , currentValueChanged()
    , currentValue()
{
    this->resize(16, 100);

    this->currentValueAnimation.setDuration(300);
    this->currentValueAnimation.setEasingCurve(
        QEasingCurve(QEasingCurve::OutCubic));

    this->setMouseTracking(true);
}

ScrollBar::~ScrollBar()
{
    auto highlight = this->highlights;

    while (highlight != NULL) {
        auto tmp = highlight->next;
        delete highlight;
        highlight = tmp;
    }
}

void
ScrollBar::removeHighlightsWhere(std::function<bool(ScrollBarHighlight &)> func)
{
    this->mutex.lock();

    ScrollBarHighlight *last = NULL;
    ScrollBarHighlight *current = this->highlights;

    while (current != NULL) {
        if (func(*current)) {
            if (last == NULL) {
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

void
ScrollBar::addHighlight(ScrollBarHighlight *highlight)
{
    this->mutex.lock();

    if (this->highlights == NULL) {
        this->highlights = highlight;
    } else {
        highlight->next = this->highlights->next;
        this->highlights->next = highlight;
    }

    this->mutex.unlock();
}

void
ScrollBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), ColorScheme::getInstance().ScrollbarBG);

    painter.fillRect(QRect(0, 0, width(), this->buttonHeight),
                     QColor(255, 0, 0));
    painter.fillRect(
        QRect(0, height() - this->buttonHeight, width(), this->buttonHeight),
        QColor(255, 0, 0));

    painter.fillRect(this->thumbRect, QColor(0, 255, 255));

    ScrollBarHighlight *highlight = this->highlights;

    this->mutex.lock();

    //    do {
    //        painter.fillRect();
    //    } while ((highlight = highlight->next()) != NULL);

    this->mutex.unlock();
}

void
ScrollBar::mouseMoveEvent(QMouseEvent *event)
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
            this->update();
        }
    } else if (this->mouseDownIndex == 2) {
        int delta = event->pos().y() - lastMousePosition.y();

        this->setDesiredValue(this->desiredValue +
                              (qreal)delta / this->trackHeight * maximum);
    }

    this->lastMousePosition = event->pos();
}

void
ScrollBar::mousePressEvent(QMouseEvent *event)
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

void
ScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    int y = event->pos().y();

    if (y < this->buttonHeight) {
        if (this->mouseDownIndex == 0) {
            this->setDesiredValue(this->desiredValue - this->smallChange, true);
        }
    } else if (y < this->thumbRect.y()) {
        if (this->mouseDownIndex == 1) {
            this->setDesiredValue(this->desiredValue - this->smallChange, true);
        }
    } else if (this->thumbRect.contains(2, y)) {
        // do nothing
    } else if (y < height() - this->buttonHeight) {
        if (this->mouseDownIndex == 3) {
            this->setDesiredValue(this->desiredValue + this->smallChange, true);
        }
    } else {
        if (this->mouseDownIndex == 4) {
            this->setDesiredValue(this->desiredValue + this->smallChange, true);
        }
    }

    this->mouseDownIndex = -1;
    update();
}

void
ScrollBar::leaveEvent(QEvent *)
{
    this->mouseOverIndex = -1;

    update();
}

void
ScrollBar::updateScroll()
{
    this->trackHeight = height() - this->buttonHeight - this->buttonHeight -
                        MIN_THUMB_HEIGHT - 1;

    this->thumbRect =
        QRect(0,
              (int)(this->currentValue / this->maximum * this->trackHeight) +
                  1 + this->buttonHeight,
              width(),
              (int)(this->largeChange / this->maximum * this->trackHeight) +
                  MIN_THUMB_HEIGHT);

    update();
}
}
}
