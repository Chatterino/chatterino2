#include "attachedwindow.hpp"

#include <QTimer>
#include <QVBoxLayout>

#include "widgets/split.hpp"

#include "Windows.h"
#pragma comment(lib, "Dwmapi.lib")

namespace chatterino {
namespace widgets {

AttachedWindow::AttachedWindow(void *_target, int _yOffset)
    : target(_target)
    , yOffset(_yOffset)
    , QWidget(nullptr, Qt::FramelessWindowHint | Qt::Window)
{
    QLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    this->setLayout(layout);

    auto *split = new Split(singletons::ThemeManager::getInstance(), this);
    this->ui.split = split;
    split->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    layout->addWidget(split);
}

AttachedWindow::~AttachedWindow()
{
    for (auto it = items.begin(); it != items.end(); it++) {
        if (it->window == this) {
            items.erase(it);
            break;
        }
    }
}

AttachedWindow *AttachedWindow::get(void *target, const QString &winId, int yOffset)
{
    for (Item &item : items) {
        if (item.hwnd == target) {
            return item.window;
        }
    }

    auto *window = new AttachedWindow(target, yOffset);
    items.push_back(Item{target, window, winId});
    return window;
}

void AttachedWindow::detach(const QString &winId)
{
    for (Item &item : items) {
        if (item.winId == winId) {
            item.window->deleteLater();
        }
    }
}

void AttachedWindow::setChannel(ChannelPtr channel)
{
    this->ui.split->setChannel(channel);
}

void AttachedWindow::showEvent(QShowEvent *)
{
    attachToHwnd(this->target);
}

void AttachedWindow::attachToHwnd(void *_hwnd)
{
    QTimer *timer = new QTimer(this);
    timer->setInterval(1);

    HWND hwnd = (HWND)this->winId();
    HWND attached = (HWND)_hwnd;
    QObject::connect(timer, &QTimer::timeout, [this, hwnd, attached, timer] {
        ::SetLastError(0);
        RECT xD;
        ::GetWindowRect(attached, &xD);

        if (::GetLastError() != 0) {
            timer->stop();
            timer->deleteLater();
            this->deleteLater();
        }

        HWND next = ::GetNextWindow(attached, GW_HWNDPREV);

        ::SetWindowPos(hwnd, next ? next : HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        ::MoveWindow(hwnd, xD.right - 360, xD.top + this->yOffset - 8, 360 - 8,
                     xD.bottom - xD.top - this->yOffset, false);
        //        ::MoveWindow(hwnd, xD.right - 360, xD.top + 82, 360 - 8, xD.bottom - xD.top - 82 -
        //        8,
        //                     false);
    });
    timer->start();
}

// void AttachedWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
//{
//    MSG *msg = reinterpret_cast

//    case WM_NCCALCSIZE: {
//    }
//}

std::vector<AttachedWindow::Item> AttachedWindow::items;

}  // namespace widgets
}  // namespace chatterino
