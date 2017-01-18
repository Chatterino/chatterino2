#include "widgets/scrollbar.h"
#include "colorscheme.h"

#include <QPainter>

#define MIN_THUMB_HEIGHT 10

namespace chatterino {
namespace widgets {

ScrollBar::ScrollBar(QWidget *widget)
    : QWidget(widget)
    , mutex()
    , highlights(NULL)
    , thumbRect()
    , maximum()
    , minimum()
    , largeChange()
    , smallChange()
    , value()
{
    resize(16, 100);
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
    painter.fillRect(rect(), ColorScheme::instance().ScrollbarBG);

    painter.fillRect(QRect(0, 0, width(), this->buttonHeight),
                     QColor(255, 0, 0));
    painter.fillRect(
        QRect(0, height() - this->buttonHeight, width(), this->buttonHeight),
        QColor(255, 0, 0));

    ScrollBarHighlight *highlight = this->highlights;

    this->mutex.lock();

    //    do {
    //        painter.fillRect();
    //    } while ((highlight = highlight->next()) != NULL);

    this->mutex.unlock();
}

void
ScrollBar::updateScroll()
{
    this->trackHeight = height() - this->buttonHeight - this->buttonHeight -
                        MIN_THUMB_HEIGHT - 1;

    this->thumbRect =
        QRect(0,
              (int)(this->value / this->maximum * this->trackHeight) + 1 +
                  this->buttonHeight,
              width(),
              (int)(this->largeChange / this->maximum * this->trackHeight) +
                  MIN_THUMB_HEIGHT);

    repaint();
}
}
}
