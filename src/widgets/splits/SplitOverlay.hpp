#pragma once

#include "widgets/BaseWidget.hpp"
#include "widgets/splits/SplitCommon.hpp"

#include <QPushButton>

#include <optional>

namespace chatterino {

class Split;

/// Type of button in the split overlay (the overlay that appears when holding down ctrl+alt)
enum class SplitOverlayButton {
    Move,
    Left,
    Up,
    Right,
    Down,
};

class SplitOverlay : public BaseWidget
{
public:
    explicit SplitOverlay(Split *parent);

    // Called from the Split Overlay's button when it gets hovered over
    void setHoveredButton(std::optional<SplitOverlayButton> hoveredButton);

    // Called from the Split Overlay's button when the move button is pressed
    void dragPressed();

    // Called from the Split Overlay's button when one of the direction buttons are pressed
    void createSplitPressed(SplitDirection direction);

protected:
    void scaleChangedEvent(float newScale) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    std::optional<SplitOverlayButton> hoveredButton_{};
    Split *split_;
    QPushButton *move_;
    QPushButton *left_;
    QPushButton *up_;
    QPushButton *right_;
    QPushButton *down_;
};

}  // namespace chatterino
