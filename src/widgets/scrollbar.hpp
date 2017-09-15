#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/scrollbarhighlight.hpp"

#include <QMutex>
#include <QPropertyAnimation>
#include <QWidget>
#include <boost/signals2.hpp>

namespace chatterino {

class ColorScheme;

namespace widgets {

class ChannelView;

class ScrollBar : public BaseWidget
{
    Q_OBJECT

public:
    ScrollBar(ChannelView *parent = 0);
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

    int _mouseOverIndex = -1;
    int _mouseDownIndex = -1;
    QPoint _lastMousePosition;

    int _buttonHeight = 16;
    int _trackHeight = 100;

    QRect _thumbRect;

    qreal _maximum = 0;
    qreal _minimum = 0;
    qreal _largeChange = 0;
    qreal _smallChange = 5;
    qreal _desiredValue = 0;
    qreal _currentValue = 0;

    boost::signals2::signal<void()> _currentValueChanged;

    void updateScroll();
};

}  // namespace widgets
}  // namespace chatterino
