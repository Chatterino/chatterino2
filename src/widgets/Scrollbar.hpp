#pragma once

#include "messages/LimitedQueue.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <pajlada/signals/signal.hpp>
#include <QMutex>
#include <QPropertyAnimation>
#include <QWidget>

namespace chatterino {

class ChannelView;

class Scrollbar : public BaseWidget
{
    Q_OBJECT

public:
    Scrollbar(size_t messagesLimit, ChannelView *parent = nullptr);

    void addHighlight(ScrollbarHighlight highlight);
    void addHighlightsAtStart(
        const std::vector<ScrollbarHighlight> &highlights_);
    void replaceHighlight(size_t index, ScrollbarHighlight replacement);

    void pauseHighlights();
    void unpauseHighlights();
    void clearHighlights();

    void scrollToBottom(bool animate = false);
    void scrollToTop(bool animate = false);
    bool isAtBottom() const;

    void setMaximum(qreal value);
    void offsetMaximum(qreal value);
    void resetMaximum();
    void setMinimum(qreal value);
    void offsetMinimum(qreal value);
    void setLargeChange(qreal value);
    void setSmallChange(qreal value);
    void setDesiredValue(qreal value, bool animated = false);
    qreal getMaximum() const;
    qreal getMinimum() const;
    qreal getLargeChange() const;
    qreal getBottom() const;
    qreal getSmallChange() const;
    qreal getDesiredValue() const;
    qreal getCurrentValue() const;
    qreal getRelativeCurrentValue() const;

    // offset the desired value without breaking smooth scolling
    void offset(qreal value);
    pajlada::Signals::NoArgSignal &getCurrentValueChanged();
    pajlada::Signals::NoArgSignal &getDesiredValueChanged();
    void setCurrentValue(qreal value);

    void printCurrentState(const QString &prefix = QString()) const;

    Q_PROPERTY(qreal desiredValue_ READ getDesiredValue WRITE setDesiredValue)

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *) override;

private:
    Q_PROPERTY(qreal currentValue_ READ getCurrentValue WRITE setCurrentValue)

    LimitedQueueSnapshot<ScrollbarHighlight> &getHighlightSnapshot();
    void updateScroll();

    QMutex mutex_;

    QPropertyAnimation currentValueAnimation_;

    LimitedQueue<ScrollbarHighlight> highlights_;
    bool highlightsPaused_{false};
    LimitedQueueSnapshot<ScrollbarHighlight> highlightSnapshot_;

    bool atBottom_{false};

    int mouseOverIndex_ = -1;
    int mouseDownIndex_ = -1;
    QPoint lastMousePosition_;

    int buttonHeight_ = 0;
    int trackHeight_ = 100;

    QRect thumbRect_;

    qreal maximum_ = 0;
    qreal minimum_ = 0;
    qreal largeChange_ = 0;
    qreal smallChange_ = 5;
    qreal desiredValue_ = 0;
    qreal currentValue_ = 0;

    pajlada::Signals::NoArgSignal currentValueChanged_;
    pajlada::Signals::NoArgSignal desiredValueChanged_;
};

}  // namespace chatterino
