#pragma once

#include "messages/limitedqueue.hpp"
#include "singletons/settingsmanager.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/scrollbarhighlight.hpp"

#include <QMutex>
#include <QPropertyAnimation>
#include <QWidget>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

namespace widgets {

class ChannelView;

class Scrollbar : public BaseWidget
{
    Q_OBJECT

public:
    Scrollbar(ChannelView *parent = 0);

    void addHighlight(ScrollbarHighlight highlight);
    void addHighlightsAtStart(const std::vector<ScrollbarHighlight> &highlights);
    void replaceHighlight(size_t index, ScrollbarHighlight replacement);

    void scrollToBottom(bool animate = false);
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
    // offset the desired value without breaking smooth scolling
    void offset(qreal value);
    pajlada::Signals::NoArgSignal &getCurrentValueChanged();
    void setCurrentValue(qreal value);

    void printCurrentState(const QString &prefix = QString()) const;

    Q_PROPERTY(qreal desiredValue READ getDesiredValue WRITE setDesiredValue)

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *) override;

private:
    Q_PROPERTY(qreal currentValue READ getCurrentValue WRITE setCurrentValue)

    QMutex mutex;

    QPropertyAnimation currentValueAnimation;

    messages::LimitedQueue<ScrollbarHighlight> highlights;

    bool atBottom = false;

    int mouseOverIndex = -1;
    int mouseDownIndex = -1;
    QPoint lastMousePosition;

    int buttonHeight = 0;
    int trackHeight = 100;

    QRect thumbRect;

    qreal maximum = 0;
    qreal minimum = 0;
    qreal largeChange = 0;
    qreal smallChange = 5;
    qreal desiredValue = 0;
    qreal currentValue = 0;
    qreal smoothScrollingOffset = 0;

    pajlada::Signals::NoArgSignal currentValueChanged;

    pajlada::Settings::Setting<bool> &smoothScrollingSetting;

    void updateScroll();
};

}  // namespace widgets
}  // namespace chatterino
