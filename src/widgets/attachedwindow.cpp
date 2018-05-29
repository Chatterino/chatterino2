#include "attachedwindow.hpp"

#include "application.hpp"

#include <QTimer>
#include <QVBoxLayout>

#include "widgets/split.hpp"

#ifdef USEWINSDK
#include "Windows.h"

#include "Psapi.h"
#pragma comment(lib, "Dwmapi.lib")
#endif

namespace chatterino {
namespace widgets {

AttachedWindow::AttachedWindow(void *_target, int _yOffset)
    : QWidget(nullptr, Qt::FramelessWindowHint | Qt::Window)
    , target(_target)
    , yOffset(_yOffset)
{
    QLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    this->setLayout(layout);

    auto *split = new Split(this);
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

AttachedWindow *AttachedWindow::get(void *target, const GetArgs &args)
{
    AttachedWindow *window = [&]() {
        for (Item &item : items) {
            if (item.hwnd == target) {
                return item.window;
            }
        }

        auto *window = new AttachedWindow(target, args.yOffset);
        items.push_back(Item{target, window, args.winId});
        return window;
    }();

    bool show = true;
    QSize size = window->size();

    if (args.height != -1) {
        if (args.height == 0) {
            window->hide();
            show = false;
        } else {
            window->_height = args.height;
            size.setHeight(args.height);
        }
    }
    if (args.width != -1) {
        if (args.width == 0) {
            window->hide();
            show = false;
        } else {
            window->_width = args.width;
            size.setWidth(args.width);
        }
    }

    if (show) {
        window->show();
        // window->resize(size);
    }

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
#ifdef USEWINSDK
    QTimer *timer = new QTimer(this);
    timer->setInterval(1);

    HWND hwnd = HWND(this->winId());
    HWND attached = HWND(_hwnd);

    QObject::connect(timer, &QTimer::timeout, [this, hwnd, attached, timer] {

        // check process id
        DWORD processId;
        ::GetWindowThreadProcessId(attached, &processId);

        HANDLE process =
            ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, processId);

        std::unique_ptr<TCHAR[]> filename(new TCHAR[512]);
        DWORD filenameLength = ::GetModuleFileNameEx(process, nullptr, filename.get(), 512);
        QString qfilename = QString::fromWCharArray(filename.get(), filenameLength);

        if (!qfilename.endsWith("chrome.exe")) {
            qDebug() << "NM Illegal callee" << qfilename;
            timer->stop();
            timer->deleteLater();
            this->deleteLater();
            return;
        }

        // We get the window rect first so we can close this window when it returns an error.
        // If we query the process first and check the filename then it will return and empty string
        // that doens't match.
        ::SetLastError(0);
        RECT rect;
        ::GetWindowRect(attached, &rect);

        if (::GetLastError() != 0) {
            timer->stop();
            timer->deleteLater();
            this->deleteLater();
            return;
        }

        // set the correct z-order
        HWND next = ::GetNextWindow(attached, GW_HWNDPREV);

        ::SetWindowPos(hwnd, next ? next : HWND_TOPMOST, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        if (this->_height == -1) {
            //            ::MoveWindow(hwnd, rect.right - this->_width - 8, rect.top + this->yOffset
            //            - 8,
            //                         this->_width, rect.bottom - rect.top - this->yOffset, false);
        } else {
            ::MoveWindow(hwnd, rect.right - this->_width - 8, rect.bottom - this->_height - 8,
                         this->_width, this->_height, false);
        }

        //        ::MoveWindow(hwnd, rect.right - 360, rect.top + 82, 360 - 8, rect.bottom -
        //        rect.top - 82 - 8, false);
    });
    timer->start();
#endif
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
