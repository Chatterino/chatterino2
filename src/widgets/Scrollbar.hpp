#pragma once

#include "widgets/BaseWidget.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <boost/circular_buffer.hpp>
#include <pajlada/signals/signal.hpp>
#include <QPropertyAnimation>
#include <QWidget>

namespace chatterino {

class ChannelView;

/// @brief A scrollbar for views with partially laid out items
///
/// This scrollbar is made for views that only lay out visible items. This is
/// the case for a @a ChannelView for example. There, only the visible messages
/// are laid out. For a traditional scrollbar, all messages would need to be
/// laid out to be able to compute the total height of all items. However, for
/// these messages this isn't possible.
///
/// To avoid having to lay out all items, this scrollbar tracks the position of
/// the content in messages (as opposed to pixels). The position is given by
/// `currentValue` which refers to the index of the message at the top plus a
/// fraction inside the message. The position can be animated to have a smooth
/// scrolling effect. In this case, `currentValue` refers to the displayed
/// position and `desiredValue` refers to the position the scrollbar is set to
/// be at after the animation. The latter is used for example to check if the
/// scrollbar is at the bottom.
///
/// @cond src-only
///
/// The following illustrates a scrollbar in a channel view with seven
/// messages. The scrollbar is at the bottom. No animation is active, thus
/// `currentValue = desiredValue`.
///
///                ┌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┐←╌╌╌ minimum
///                  Alice: This message is quite           = 0
///             ┬  ╭─────────────────────────────────╮←╮
///             │  │ long, so it gets wrapped        │ ┆
///             │  ├╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┤ ╰╌╌╌ currentValue
///             │  │ Bob: are you sure?              │       = 0.5
///             │  ├╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┤       = desiredValue
/// pageSize ╌╌╌┤  │ Alice: Works for me... try for  │       = maximum
///  = 6.5      │  │ yourself                        │         - pageSize
///             │  ├╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┤       = bottom
///             │  │ Bob: I'm trying to get my really│       ⇒ atBottom = true
///             │  │ long message to wrap so I can   │
///             │  │ debug this issue I'm facing...  │
///             │  ├╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┤
///             │  │ Bob: Omg it worked              │
///             │  ├╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌┤
///             │  │ Alice: That's amazing!         ╭┤ ┬
///             │  │╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌││ ├╌╌ thumbRect.height()
///             │  │ Bob: you're right              ╰┤ ┴
///             ┴╭→╰─────────────────────────────────╯
///              ┆
///           maximum
///            = 7
/// @endcond
class Scrollbar : public BaseWidget
{
    Q_OBJECT

public:
    Scrollbar(size_t messagesLimit, ChannelView *parent = nullptr);

    void addHighlight(ScrollbarHighlight highlight);
    void addHighlightsAtStart(
        const std::vector<ScrollbarHighlight> &highlights_);
    void replaceHighlight(size_t index, ScrollbarHighlight replacement);

    void clearHighlights();

    void scrollToBottom(bool animate = false);
    void scrollToTop(bool animate = false);
    bool isAtBottom() const;

    void setMaximum(qreal value);
    void offsetMaximum(qreal value);
    void resetMaximum();
    void setMinimum(qreal value);
    void offsetMinimum(qreal value);
    void setPageSize(qreal value);
    void setDesiredValue(qreal value, bool animated = false);
    qreal getMaximum() const;
    qreal getMinimum() const;
    qreal getPageSize() const;
    qreal getBottom() const;
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
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    Q_PROPERTY(qreal currentValue_ READ getCurrentValue WRITE setCurrentValue)

    void updateScroll();

    QPropertyAnimation currentValueAnimation_;

    boost::circular_buffer<ScrollbarHighlight> highlights_;

    bool atBottom_{false};

    int mouseOverIndex_ = -1;
    int mouseDownIndex_ = -1;
    QPoint lastMousePosition_;

    int trackHeight_ = 100;

    QRect thumbRect_;

    qreal maximum_ = 0;
    qreal minimum_ = 0;
    qreal pageSize_ = 0;
    qreal desiredValue_ = 0;
    qreal currentValue_ = 0;

    pajlada::Signals::NoArgSignal currentValueChanged_;
    pajlada::Signals::NoArgSignal desiredValueChanged_;
};

}  // namespace chatterino
