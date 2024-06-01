#pragma once

class QWidget;
class QPoint;
class QScreen;
class QRect;

namespace chatterino::widgets {

enum class BoundsChecking {
    /// Don't do any bounds checking (equivalent to `QWidget::move`).
    Off,

    /// Attempt to keep the window within bounds of the screen the cursor is on.
    CursorPosition,

    /// Attempt to keep the window within bounds of the screen the desired position is on.
    DesiredPosition,
};

/// Applies bounds checking to @a initialBounds.
///
/// @param initialBounds The bounds to check.
/// @param mode The desired bounds checking.
/// @returns The potentially modified bounds.
QRect checkInitialBounds(QRect initialBounds,
                         BoundsChecking mode = BoundsChecking::DesiredPosition);

/// Moves the `window` to the (global) `position`
/// while doing bounds-checking according to `mode` to ensure the window stays on one screen.
///
/// @param window The window to move.
/// @param position The global position to move the window to.
/// @param mode The desired bounds checking.
void moveWindowTo(QWidget *window, QPoint position,
                  BoundsChecking mode = BoundsChecking::DesiredPosition);

/// Moves the `window` to the (global) `position`
/// while doing bounds-checking according to `mode` to ensure the window stays on one screen.
/// Will also call show on the `window`, order is dependant on platform.
///
/// @param window The window to move.
/// @param position The global position to move the window to.
/// @param mode The desired bounds checking.
void showAndMoveWindowTo(QWidget *window, QPoint position,
                         BoundsChecking mode = BoundsChecking::DesiredPosition);

}  // namespace chatterino::widgets
