#pragma once

#include "widgets/BaseWidget.hpp"
#include "widgets/helper/ScrollbarHighlight.hpp"

#include <boost/circular_buffer.hpp>
#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>
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
/// `minimum` and `maximum` are used to map scrollbar positions to
/// (message-)buffer indices. The buffer is of size `maximum - minimum` and an
/// index is computed by `scrollbarPos - minimum` - thus a scrollbar position
/// of a message is at `index + minimum.
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
///
/// When messages are added at the bottom, both maximum and minimum are offset
/// by 1 and after a layout, the desired value is updated, causing the content
/// to move. Afterwards, the bounds are reset (potentially waiting for the
/// animation to finish).
///
/// While scrolling is paused, the desired (and current) value won't be
/// updated. However, messages can still come in and "shift" the values in the
/// backing ring-buffer. If the current value would be used, the messages would
/// still shift upwards (just at a different offset). To avoid this, there's a
/// _relative current value_, which is `currentValue - minimum`. It's the
/// actual index of the top message in the buffer. Since the minimum is shifted
/// by 1 when messages come in, the view will remain idle (visually).
class Scrollbar : public BaseWidget
{
    Q_OBJECT

public:
    Scrollbar(size_t messagesLimit, ChannelView *parent);

    /// Return a copy of the highlights
    ///
    /// Should only be used for tests
    boost::circular_buffer<ScrollbarHighlight> getHighlights() const;
    void addHighlight(ScrollbarHighlight highlight);
    void addHighlightsAtStart(
        const std::vector<ScrollbarHighlight> &highlights_);
    void replaceHighlight(size_t index, ScrollbarHighlight replacement);

    void clearHighlights();

    void scrollToBottom(bool animate = false);
    void scrollToTop(bool animate = false);
    bool isAtBottom() const;

    qreal getMaximum() const;
    void setMaximum(qreal value);
    void offsetMaximum(qreal value);

    qreal getMinimum() const;
    void setMinimum(qreal value);
    void offsetMinimum(qreal value);

    void resetBounds();

    qreal getPageSize() const;
    void setPageSize(qreal value);

    qreal getDesiredValue() const;
    void setDesiredValue(qreal value, bool animated = false);

    /// The bottom-most scroll position
    qreal getBottom() const;
    qreal getCurrentValue() const;

    /// @brief The current value relative to the minimum
    ///
    /// > currentValue - minimum
    ///
    /// This should be used as an index into a buffer of messages, as it is
    /// unaffected by simultaneous shifts of minimum and maximum.
    qreal getRelativeCurrentValue() const;

    void setHideThumb(bool hideThumb);

    /// Returns true if we should show the thumb (the handle you can drag)
    bool shouldShowThumb() const;

    void setHideHighlights(bool hideHighlights);

    /// Returns true if we should show the highlights
    bool shouldShowHighlights() const;

    bool shouldHandleMouseEvents() const;

    // offset the desired value without breaking smooth scolling
    void offset(qreal value);
    pajlada::Signals::NoArgSignal &getCurrentValueChanged();
    pajlada::Signals::NoArgSignal &getDesiredValueChanged();
    void setCurrentValue(qreal value);

    void printCurrentState(
        const QString &prefix = QStringLiteral("Scrollbar")) const;

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

    enum class MouseLocation {
        /// The mouse is positioned outside the scrollbar
        Outside,
        /// The mouse is positioned inside the scrollbar, but above the thumb (the thing you can drag inside the scrollbar)
        AboveThumb,
        /// The mouse is positioned inside the scrollbar, and on top of the thumb
        InsideThumb,
        /// The mouse is positioned inside the scrollbar, but below the thumb
        BelowThumb,
    };

    MouseLocation locationOfMouseEvent(QMouseEvent *event) const;

    QPropertyAnimation currentValueAnimation_;

    boost::circular_buffer<ScrollbarHighlight> highlights_;

    bool atBottom_{true};
    /// This takes precedence over `settingHideThumb`
    bool hideThumb{false};
    /// Controlled by the "Hide scrollbar thumb" setting
    bool settingHideThumb{false};

    /// This takes precedence over `settingHideHighlights`
    bool hideHighlights = false;
    /// Controlled by the "Hide scrollbar highlights" setting
    bool settingHideHighlights{false};

    MouseLocation mouseOverLocation_ = MouseLocation::Outside;
    MouseLocation mouseDownLocation_ = MouseLocation::Outside;
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

    pajlada::Signals::SignalHolder signalHolder;
};

}  // namespace chatterino
