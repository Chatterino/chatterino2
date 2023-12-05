#include "util/WidgetHelpers.hpp"

#include <QCursor>
#include <QGuiApplication>
#include <QPoint>
#include <QScreen>
#include <QWidget>

namespace {

/// Move the `window` into the `screen` geometry if it's not already in there.
void moveWithinScreen(QWidget *window, QScreen *screen, QPoint point)
{
    if (screen == nullptr)
    {
        screen = QGuiApplication::primaryScreen();
    }

    const QRect bounds = screen->availableGeometry();

    bool stickRight = false;
    bool stickBottom = false;

    const auto w = window->frameGeometry().width();
    const auto h = window->frameGeometry().height();

    if (point.x() < bounds.left())
    {
        point.setX(bounds.left());
    }
    if (point.y() < bounds.top())
    {
        point.setY(bounds.top());
    }
    if (point.x() + w > bounds.right())
    {
        stickRight = true;
        point.setX(bounds.right() - w);
    }
    if (point.y() + h > bounds.bottom())
    {
        stickBottom = true;
        point.setY(bounds.bottom() - h);
    }

    if (stickRight && stickBottom)
    {
        const QPoint globalCursorPos = QCursor::pos();
        point.setY(globalCursorPos.y() - window->height() - 16);
    }

    window->move(point);
}

}  // namespace

namespace chatterino::widgets {

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
