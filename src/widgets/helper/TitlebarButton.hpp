#pragma once

#include "widgets/helper/Button.hpp"

namespace chatterino {

enum class TitleBarButtonStyle {
    None = 0,
    Minimize = 1,
    Maximize = 2,
    Unmaximize = 4,
    Close = 8,
    User = 16,
    Settings = 32,
    StreamerMode = 64,
};

class TitleBarButton : public Button
{
public:
    TitleBarButton();

    TitleBarButtonStyle getButtonStyle() const;
    void setButtonStyle(TitleBarButtonStyle style_);

    /// Simulate a `mouseEnter` event.
    void ncEnter();

    /// Simulate a `mouseLeave` event.
    void ncLeave();

    /// Simulate a `mouseMove` event.
    /// @param at a local position relative to this widget
    void ncMove(QPoint at);

    /// Simulate a `mousePress` event with the left mouse button.
    /// @param at a local position relative to this widget
    void ncMousePress(QPoint at);

    /// Simulate a `mouseRelease` event with the left mouse button.
    /// @param at a local position relative to this widget
    void ncMouseRelease(QPoint at);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    TitleBarButtonStyle style_{};
};

}  // namespace chatterino
