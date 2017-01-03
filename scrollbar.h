#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include <functional>
#include "QWidget"
#include "QMutex"
#include "scrollbarhighlight.h"

class ScrollBar : public QWidget
{
    Q_OBJECT

public:
    ScrollBar(QWidget* parent = 0);
    ~ScrollBar();

    void removeHighlightsWhere(std::function<bool (ScrollBarHighlight&)> func);
    void addHighlight(ScrollBarHighlight* highlight);

private:
    QMutex mutex;
    ScrollBarHighlight* highlights = NULL;
    void paintEvent(QPaintEvent *);

    QRect thumbRect;
};

#endif // SCROLLBAR_H
