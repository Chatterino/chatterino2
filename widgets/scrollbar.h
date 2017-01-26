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

    Q_PROPERTY(qreal value READ getValue WRITE setValue)

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
    setValue(qreal value, bool animated = false)
    {
        value = std::max(this->minimum,
                         std::min(this->maximum - this->largeChange, value));

        if (this->value != value) {
            if (animated) {
                this->valueAnimation.stop();
                this->valueAnimation.setStartValue(this->value);

                this->valueAnimation.setEndValue(value);
                this->valueAnimation.start();
            } else {
                this->value = value;
            }

            this->updateScroll();
            this->valueChanged();

            this->repaint();
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
    getValue() const
    {
        return this->value;
    }

    boost::signals2::signal<void()> &
    getValueChanged()
    {
        return valueChanged;
    }

private:
    QMutex mutex;
    ScrollBarHighlight *highlights;

    QPropertyAnimation valueAnimation;

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
    qreal value;

    boost::signals2::signal<void()> valueChanged;

    void updateScroll();
};
}
}

#endif  // SCROLLBAR_H
