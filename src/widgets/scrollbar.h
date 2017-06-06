#pragma once

#include "widgets/scrollbarhighlight.h"

#include <QMutex>
#include <QPropertyAnimation>
#include <QWidget>
#include <boost/signals2.hpp>

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

    Q_PROPERTY(qreal _desiredValue READ getDesiredValue WRITE setDesiredValue)

    void scrollToBottom();

    bool isAtBottom() const;

    void setMaximum(qreal value);
    void setMinimum(qreal value);
    void setLargeChange(qreal value);
    void setSmallChange(qreal value);
    void setDesiredValue(qreal value, bool animated = false);
    qreal getMaximum() const;
    qreal getMinimum() const;
    qreal getLargeChange() const;
    qreal getSmallChange() const;
    qreal getDesiredValue() const;
    qreal getCurrentValue() const;
    boost::signals2::signal<void()> &getCurrentValueChanged();
    void setCurrentValue(qreal value);

    void printCurrentState(const QString &prefix = QString()) const;

private:
    Q_PROPERTY(qreal _currentValue READ getCurrentValue WRITE setCurrentValue)

    QMutex _mutex;

    QPropertyAnimation _currentValueAnimation;

    ScrollBarHighlight *_highlights;

    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);

    int _mouseOverIndex;
    int _mouseDownIndex;
    QPoint _lastMousePosition;

    int _buttonHeight;
    int _trackHeight;

    QRect _thumbRect;

    qreal _maximum;
    qreal _minimum;
    qreal _largeChange;
    qreal _smallChange;
    qreal _desiredValue;
    qreal _currentValue;

    boost::signals2::signal<void()> _currentValueChanged;

    void updateScroll();
};

}  // namespace widgets
}  // namespace chatterino
