#include "QPainter"
#include "scrollbar.h"
#include "colorscheme.h"

ScrollBar::ScrollBar(QWidget* widget)
    : QWidget(widget),
      mutex()
{
    resize(16, 100);
}

ScrollBar::~ScrollBar()
{
    auto highlight = highlights;

    while (highlight != NULL)
    {
        auto tmp = highlight->next;
        delete highlight;
        highlight = tmp;
    }
}

void ScrollBar::removeHighlightsWhere(std::function<bool (ScrollBarHighlight&)> func)
{
    mutex.lock();

    ScrollBarHighlight* last = NULL;
    ScrollBarHighlight* current = highlights;

    while (current != NULL)
    {
        if (func(*current))
        {
            if (last == NULL)
            {
                highlights = current->next;
            }
            else
            {
                last->next = current->next;
            }

            auto oldCurrent = current;

            current = current->next;
            last = current;

            delete oldCurrent;
        }
    }

    mutex.unlock();
}

void ScrollBar::addHighlight(ScrollBarHighlight* highlight)
{
    mutex.lock();

    if (highlights == NULL)
    {
        highlights = highlight;
    }
    else
    {
        highlight->next = highlights->next;
        highlights->next = highlight;
    }

    mutex.unlock();
}

void ScrollBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), ColorScheme::instance().ScrollbarBG);

    mutex.lock();



    mutex.unlock();
}
