#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "scrollbarhighlight.h"

#include <QMutex>
#include <QWidget>
#include <functional>

class ScrollBar : public QWidget
{
    Q_OBJECT

public:
    ScrollBar(QWidget *parent = 0);
    ~ScrollBar();

    void removeHighlightsWhere(std::function<bool(ScrollBarHighlight &)> func);
    void addHighlight(ScrollBarHighlight *highlight);

    void
    setMaximum(qreal value)
    {
        maximum = value;

        updateScroll();
    }

    void
    setMinimum(qreal value)
    {
        minimum = value;

        updateScroll();
    }

    void
    setLargeChange(qreal value)
    {
        largeChange = value;

        updateScroll();
    }

    void
    setSmallChange(qreal value)
    {
        smallChange = value;

        updateScroll();
    }

    void
    setValue(qreal value)
    {
        value = value;

        updateScroll();
    }

    qreal
    getMaximum() const
    {
        return maximum;
    }

    qreal
    getMinimum() const
    {
        return minimum;
    }

    qreal
    getLargeChange() const
    {
        return largeChange;
    }

    qreal
    getSmallChange() const
    {
        return smallChange;
    }

    qreal
    getValue() const
    {
        return value;
    }

private:
    QMutex mutex;
    ScrollBarHighlight *highlights;
    void paintEvent(QPaintEvent *);

    int buttonHeight;
    int trackHeight;

    QRect thumbRect;

    qreal maximum;
    qreal minimum;
    qreal largeChange;
    qreal smallChange;
    qreal value;

    void updateScroll();
};

#endif  // SCROLLBAR_H
