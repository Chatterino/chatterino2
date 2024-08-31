#pragma once

class QPoint;
class QWidget;

#include <QObject>
#include <QtGlobal>

namespace chatterino {

#ifdef USEWINSDK

class TitleBarButton;
class TitleBarButtons : QObject
{
public:
    /// The parent of this object is set to `window`.
    ///
    /// All parameters must have a parent;
    /// they're not deleted in the destructor.
    TitleBarButtons(QWidget *window, TitleBarButton *minButton,
                    TitleBarButton *maxButton, TitleBarButton *closeButton);

    /// Hover over the button `ht` at the global position `at`.
    ///
    /// @pre `ht` must be one of { HTMAXBUTTON, HTMINBUTTON, HTCLOSE }.
    /// @param ht The hovered button
    /// @param at The global position of the event
    void hover(size_t ht, QPoint at);

    /// Leave all buttons - simulate `leaveEvent` for all buttons.
    void leave();

    /// Press the left mouse over the button `ht` at the global position `at`.
    ///
    /// @pre `ht` must be one of { HTMAXBUTTON, HTMINBUTTON, HTCLOSE }.
    /// @param ht The clicked button
    /// @param at The global position of the event
    void mousePress(size_t ht, QPoint at);

    /// Release the left mouse button over the button `ht` at the
    /// global position `at`.
    ///
    /// @pre `ht` must be one of { HTMAXBUTTON, HTMINBUTTON, HTCLOSE }.
    /// @param ht The clicked button
    /// @param at The global position of the event
    void mouseRelease(size_t ht, QPoint at);

    /// Update the maximize/restore button to show the correct image
    /// according to the current window state.
    void updateMaxButton();

    /// Set buttons to be narrow.
    void setSmallSize();
    /// Set buttons to be regular size.
    void setRegularSize();

private:
    /// @pre ht must be one of { HTMAXBUTTON, HTMINBUTTON, HTCLOSE }.
    TitleBarButton *buttonForHt(size_t ht) const;

    QWidget *window_ = nullptr;
    TitleBarButton *minButton_ = nullptr;
    TitleBarButton *maxButton_ = nullptr;
    TitleBarButton *closeButton_ = nullptr;
};

#endif

}  // namespace chatterino
