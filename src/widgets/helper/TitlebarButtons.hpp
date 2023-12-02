#pragma once

class QPoint;
class QWidget;

#include <QtGlobal>

namespace chatterino {

#ifdef USEWINSDK

class TitleBarButton;
class TitleBarButtons : QObject
{
public:
    TitleBarButtons(QWidget *window, TitleBarButton *minButton,
                    TitleBarButton *maxButton, TitleBarButton *closeButton);

    /// @pre ht must be one of { HTMAXBUTTON, HTMINBUTTON, HTCLOSE }.
    /// @param at The global position of the event
    void hover(size_t ht, QPoint at);

    void leave();

    /// @pre ht must be one of { HTMAXBUTTON, HTMINBUTTON, HTCLOSE }.
    /// @param at The global position of the event
    void mouseDown(size_t ht, QPoint at);

    /// @pre ht must be one of { HTMAXBUTTON, HTMINBUTTON, HTCLOSE }.
    /// @param at The global position of the event
    void mouseUp(size_t ht, QPoint at);

    void updateMaxButton();

    void setSmallSize();
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
