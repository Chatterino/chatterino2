#ifndef SCROLLBAR_H
#define SCROLLBAR_H

#include "widgets/scrollbarhighlight.h"

#include <QMutex>
#include <QPropertyAnimation>
#include <QWidget>
#include <boost/signals2.hpp>
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

    Q_PROPERTY(qreal desiredValue READ getDesiredValue WRITE setDesiredValue)

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
    setDesiredValue(qreal value, bool animated = false)
    {
        value = std::max(this->minimum,
                         std::min(this->maximum - this->largeChange, value));

        this->desiredValue = value;

        if (this->desiredValue != value) {
            if (animated) {
                this->currentValueAnimation.stop();
                this->currentValueAnimation.setStartValue(this->currentValue);

                this->currentValueAnimation.setEndValue(value);
                this->currentValueAnimation.start();
            } else {
                this->currentValueAnimation.stop();

                this->setCurrentValue(value);
            }
        }
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
    getDesiredValue() const
    {
        return this->desiredValue;
    }

    qreal
    getCurrentValue() const
    {
        return this->currentValue;
    }

    boost::signals2::signal<void()> &
    getCurrentValueChanged()
    {
        return currentValueChanged;
    }

    void
    setCurrentValue(qreal value)
    {
        value = std::max(this->minimum,
                         std::min(this->maximum - this->largeChange, value));

        if (this->currentValue != value) {
            this->currentValue = value;

            this->updateScroll();
            this->currentValueChanged();

            this->update();
        }
    }

private:
    Q_PROPERTY(qreal currentValue READ getCurrentValue WRITE setCurrentValue)

    QMutex mutex;
    ScrollBarHighlight *highlights;

    QPropertyAnimation currentValueAnimation;

    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);

    int mouseOverIndex;
    int mouseDownIndex;
    QPoint lastMousePosition;

    int buttonHeight;
    int trackHeight;

    QRect thumbRect;

    qreal maximum;
    qreal minimum;
    qreal largeChange;
    qreal smallChange;
    qreal desiredValue;
    qreal currentValue;

    boost::signals2::signal<void()> currentValueChanged;

    void updateScroll();
};
}
}

#endif  // SCROLLBAR_H
