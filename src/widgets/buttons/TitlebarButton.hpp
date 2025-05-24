#pragma once

#include "widgets/buttons/DimButton.hpp"

namespace chatterino {

enum class TitleBarButtonStyle : std::uint8_t {
    None,
    Minimize,
    Maximize,
    Unmaximize,
    Close,
    Settings,
};

class TitleBarButton : public DimButton
{
public:
    TitleBarButton(TitleBarButtonStyle style = {});

    TitleBarButtonStyle getButtonStyle() const;
    void setButtonStyle(TitleBarButtonStyle style);

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
    void themeChangedEvent() override;

    void paintContent(QPainter &painter) override;

private:
    TitleBarButtonStyle style_{};
};

}  // namespace chatterino
