#include "util/WidgetHelpers.hpp"

#include <QCursor>
#include <QGuiApplication>
#include <QPoint>
#include <QScreen>
#include <QWidget>

namespace {

QPoint applyBounds(QScreen *screen, QPoint point, QSize frameSize)
{
    if (screen == nullptr)
    {
        screen = QGuiApplication::primaryScreen();
    }

    const QRect bounds = screen->availableGeometry();

    // If the hover frame would go off the bottom of the screen it should be moved above the cursor

    if (point.x() < bounds.left())
    {
        point.setX(bounds.left());
    }
    if (point.y() < bounds.top())
    {
        point.setY(bounds.top());
    }
    if (point.x() + frameSize.width() > bounds.right())
    {
        point.setX(bounds.right() - frameSize.width());
    }
    if (point.y() + frameSize.height() > bounds.bottom())
    {
        const QPoint globalCursorPos = QCursor::pos();
        point.setY(globalCursorPos.y() - frameSize.height() - 16);
    }

    return point;
}

/// Move the `window` into the `screen` geometry if it's not already in there.
void moveWithinScreen(QWidget *window, QScreen *screen, QPoint point)
{
    auto checked = applyBounds(screen, point, window->frameSize());
    window->move(checked);
}

}  // namespace

namespace chatterino::widgets {

QRect checkInitialBounds(QRect initialBounds, BoundsChecking mode)
{
    switch (mode)
    {
        case BoundsChecking::Off: {
            return initialBounds;
        }
        break;

        case BoundsChecking::CursorPosition: {
            return QRect{
                applyBounds(QGuiApplication::screenAt(QCursor::pos()),
                            initialBounds.topLeft(), initialBounds.size()),
                initialBounds.size(),
            };
        }
        break;

        case BoundsChecking::DesiredPosition: {
            return QRect{
                applyBounds(QGuiApplication::screenAt(initialBounds.topLeft()),
                            initialBounds.topLeft(), initialBounds.size()),
                initialBounds.size(),
            };
        }
        break;
        default:
            assert(false && "Invalid bounds checking mode");
            return initialBounds;
    }
}

void moveWindowTo(QWidget *window, QPoint position, BoundsChecking mode)
{
    switch (mode)
    {
        case BoundsChecking::Off: {
            window->move(position);
        }
        break;

        case BoundsChecking::CursorPosition: {
            moveWithinScreen(window, QGuiApplication::screenAt(QCursor::pos()),
                             position);
        }
        break;

        case BoundsChecking::DesiredPosition: {
            moveWithinScreen(window, QGuiApplication::screenAt(position),
                             position);
        }
        break;
    }
}

void showAndMoveWindowTo(QWidget *window, QPoint position, BoundsChecking mode)
{
#ifdef Q_OS_WINDOWS
    window->show();

    moveWindowTo(window, position, mode);
#else
    moveWindowTo(window, position, mode);

    window->show();
#endif
}

}  // namespace chatterino::widgets
