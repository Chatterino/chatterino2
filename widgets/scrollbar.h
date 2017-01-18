#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "widgets/scrollbarhighlight.h"

#include <QMutex>
#include <QWidget>
#include <functional>

namespace chatterino {
namespace widgets {

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
        this->maximum = value;

        this->updateScroll();
    }

    void
    setMinimum(qreal value)
    {
        this->minimum = value;

        this->updateScroll();
    }

    void
    setLargeChange(qreal value)
    {
        this->largeChange = value;

        this->updateScroll();
    }

    void
    setSmallChange(qreal value)
    {
        this->smallChange = value;

        this->updateScroll();
    }

    void
    setValue(qreal value)
    {
        this->value = value;

        this->updateScroll();
    }

    qreal
    getMaximum() const
    {
        return this->maximum;
    }

    qreal
    getMinimum() const
    {
        return this->minimum;
    }

    qreal
    getLargeChange() const
    {
        return this->largeChange;
    }

    qreal
    getSmallChange() const
    {
        return this->smallChange;
    }

    qreal
    getValue() const
    {
        return this->value;
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
}
}

#endif  // SCROLLBAR_H
