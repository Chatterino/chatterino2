#pragma once

#include "singletons/settingsmanager.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/helper/scrollbarhighlight.hpp"

#include <QMutex>
#include <QPropertyAnimation>
#include <QWidget>
#include <boost/signals2.hpp>

namespace chatterino {

class ThemeManager;

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

    Q_PROPERTY(qreal desiredValue READ getDesiredValue WRITE setDesiredValue)

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
    // offset the desired value without breaking smooth scolling
    void offset(qreal value);
    boost::signals2::signal<void()> &getCurrentValueChanged();
    void setCurrentValue(qreal value);

    void printCurrentState(const QString &prefix = QString()) const;

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

    ScrollBarHighlight *highlights;

    bool atBottom = false;

    int mouseOverIndex = -1;
    int mouseDownIndex = -1;
    QPoint lastMousePosition;

    //    int buttonHeight = 16;
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

    boost::signals2::signal<void()> currentValueChanged;

    pajlada::Settings::Setting<bool> &smoothScrollingSetting;

    void updateScroll();
};

}  // namespace widgets
}  // namespace chatterino
