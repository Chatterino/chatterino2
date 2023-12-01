#pragma once

class QPoint;
class QWidget;

#include <QtGlobal>

#include <tuple>

namespace chatterino {

#ifdef USEWINSDK

class TitleBarButton;
class TitleBarButtons : QObject
{
public:
    TitleBarButtons(QWidget *window, TitleBarButton *minButton,
                    TitleBarButton *maxButton, TitleBarButton *closeButton);

    void hover(size_t ht, QPoint at);
    void leave();
    void mouseDown(size_t ht, QPoint at);
    void mouseUp(size_t ht, QPoint at);

    void updateMaxButton();

    void setSmallSize();
    void setRegularSize();

private:
    std::pair<TitleBarButton *, std::array<TitleBarButton *, 2>> buttonForHt(
        size_t ht) const;

    QWidget *window_ = nullptr;
    TitleBarButton *minButton_ = nullptr;
    TitleBarButton *maxButton_ = nullptr;
    TitleBarButton *closeButton_ = nullptr;
};

#endif

}  // namespace chatterino
