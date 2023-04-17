#include "widgets/AttachedWindow.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"
#include "widgets/splits/Split.hpp"

#include <QTimer>
#include <QVBoxLayout>

#include <memory>

#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"

// clang-format off
// don't even think about reordering these
#    include "Windows.h"
#    include "Psapi.h"
// clang-format on
#    pragma comment(lib, "Dwmapi.lib")
#endif

namespace chatterino {

#ifdef USEWINSDK
static thread_local std::vector<HWND> taskbarHwnds;

BOOL CALLBACK enumWindows(HWND hwnd, LPARAM)
{
    constexpr int length = 16;

    auto className = std::make_unique<WCHAR[]>(length);
    GetClassName(hwnd, className.get(), length);

    if (lstrcmp(className.get(), L"Shell_TrayWnd") == 0 ||
        lstrcmp(className.get(), L"Shell_Secondary") == 0)
    {
        taskbarHwnds.push_back(hwnd);
    }

    return true;
}
#endif

AttachedWindow::AttachedWindow(void *_target, int _yOffset)
    : QWidget(nullptr, Qt::FramelessWindowHint | Qt::Window)
    , target_(_target)
    , yOffset_(_yOffset)
{
    QLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    auto *split = new Split(this);
    this->ui_.split = split;
    split->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
    layout->addWidget(split);

    DebugCount::increase("attached window");
}

AttachedWindow::~AttachedWindow()
{
    for (auto it = items.begin(); it != items.end(); it++)
    {
        if (it->window == this)
        {
            items.erase(it);
            break;
        }
    }

    DebugCount::decrease("attached window");
}

AttachedWindow *AttachedWindow::get(void *target, const GetArgs &args)
{
    AttachedWindow *window = [&]() {
        for (Item &item : items)
        {
            if (item.hwnd == target)
            {
                return item.window;
            }
        }

        auto *window = new AttachedWindow(target, args.yOffset);
        items.push_back(Item{target, window, args.winId});
        return window;
    }();

    bool show = true;
    QSize size = window->size();

    window->fullscreen_ = args.fullscreen;

    window->x_ = args.x;
    window->pixelRatio_ = args.pixelRatio;

    if (args.height != -1)
    {
        if (args.height == 0)
        {
            window->hide();
            show = false;
        }
        else
        {
            window->height_ = args.height;
            size.setHeight(args.height);
        }
    }
    if (args.width != -1)
    {
        if (args.width == 0)
        {
            window->hide();
            show = false;
        }
        else
        {
            window->width_ = args.width;
            size.setWidth(args.width);
        }
    }

    if (show)
    {
        window->updateWindowRect(window->target_);
        window->show();
    }

    return window;
}

#ifdef USEWINSDK
AttachedWindow *AttachedWindow::getForeground(const GetArgs &args)
{
    return AttachedWindow::get(::GetForegroundWindow(), args);
}
#endif

void AttachedWindow::detach(const QString &winId)
{
    for (Item &item : items)
    {
        if (item.winId == winId)
        {
            item.window->deleteLater();
        }
    }
}

void AttachedWindow::setChannel(ChannelPtr channel)
{
    this->ui_.split->setChannel(std::move(channel));
}

void AttachedWindow::showEvent(QShowEvent *)
{
    this->attachToHwnd(this->target_);
}

void AttachedWindow::attachToHwnd(void *_attachedPtr)
{
#ifdef USEWINSDK
    if (this->attached_)
    {
        return;
    }

    this->attached_ = true;

    //auto hwnd = HWND(this->winId());
    auto attached = HWND(_attachedPtr);

    // FAST TIMER - used to resize/reorder windows
    this->timer_.setInterval(1);
    QObject::connect(&this->timer_, &QTimer::timeout, [this, attached] {
        // check process id
        if (!this->validProcessName_)
        {
            DWORD processId;
            ::GetWindowThreadProcessId(attached, &processId);

            HANDLE process = ::OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, processId);

            std::unique_ptr<TCHAR[]> filename(new TCHAR[512]);
            DWORD filenameLength =
                ::GetModuleFileNameEx(process, nullptr, filename.get(), 512);
            QString qfilename =
                QString::fromWCharArray(filename.get(), int(filenameLength));

            if (!getSettings()->attachExtensionToAnyProcess)
            {
                // We don't attach to non-browser processes by default.
                if (!qfilename.endsWith("chrome.exe") &&
                    !qfilename.endsWith("firefox.exe") &&
                    !qfilename.endsWith("vivaldi.exe") &&
                    !qfilename.endsWith("opera.exe") &&
                    !qfilename.endsWith("msedge.exe") &&
                    !qfilename.endsWith("brave.exe"))

                {
                    qCWarning(chatterinoWidget)
                        << "NM Illegal caller" << qfilename;
                    this->timer_.stop();
                    this->deleteLater();
                    return;
                }
            }
            this->validProcessName_ = true;
        }

        this->updateWindowRect(attached);
    });

    this->timer_.start();

    // SLOW TIMER - used to hide taskbar behind fullscreen window
    this->slowTimer_.setInterval(2000);
    QObject::connect(&this->slowTimer_, &QTimer::timeout, [this, attached] {
        if (this->fullscreen_)
        {
            taskbarHwnds.clear();
            ::EnumWindows(&enumWindows, 0);

            for (auto taskbarHwnd : taskbarHwnds)
            {
                ::SetWindowPos(taskbarHwnd,
                               GetNextWindow(attached, GW_HWNDNEXT), 0, 0, 0, 0,
                               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }
    });
    this->slowTimer_.start();
#endif
}

void AttachedWindow::updateWindowRect(void *_attachedPtr)
{
#ifdef USEWINSDK
    auto hwnd = HWND(this->winId());
    auto attached = HWND(_attachedPtr);

    // We get the window rect first so we can close this window when it returns
    // an error. If we query the process first and check the filename then it
    // will return and empty string that doens't match.
    ::SetLastError(0);
    RECT rect;
    ::GetWindowRect(attached, &rect);

    if (::GetLastError() != 0)
    {
        qCWarning(chatterinoWidget) << "NM GetLastError()" << ::GetLastError();

        this->timer_.stop();
        this->deleteLater();
        return;
    }

    // set the correct z-order
    if (HWND next = ::GetNextWindow(attached, GW_HWNDPREV))
    {
        ::SetWindowPos(hwnd, next ? next : HWND_TOPMOST, 0, 0, 0, 0,
                       SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    float scale = 1.f;
    float ourScale = 1.F;
    if (auto dpi = getWindowDpi(attached))
    {
        scale = *dpi / 96.f;
        ourScale = scale / this->devicePixelRatio();

        for (auto w : this->ui_.split->findChildren<BaseWidget *>())
        {
            w->setOverrideScale(ourScale);
        }
        this->ui_.split->setOverrideScale(ourScale);
    }

    if (this->height_ != -1)
    {
        this->ui_.split->setFixedWidth(int(this->width_ * ourScale));

        // offset
        int o = this->fullscreen_ ? 0 : 8;

        if (this->pixelRatio_ != -1.0)
        {
            ::MoveWindow(
                hwnd,
                int(rect.left + this->x_ * scale * this->pixelRatio_ + o - 2),
                int(rect.bottom - this->height_ * scale - o),
                int(this->width_ * scale), int(this->height_ * scale), true);
        }
        //support for old extension version 1.3
        else if (this->x_ != -1.0)
        {
            ::MoveWindow(hwnd, int(rect.left + this->x_ * scale + o),
                         int(rect.bottom - this->height_ * scale - o),
                         int(this->width_ * scale), int(this->height_ * scale),
                         true);
        }
        //support for old extension version 1.2
        else
        {
            ::MoveWindow(hwnd, int(rect.right - this->width_ * scale - o),
                         int(rect.bottom - this->height_ * scale - o),
                         int(this->width_ * scale), int(this->height_ * scale),
                         true);
        }
    }

//    if (this->fullscreen_)
//    {
//        ::BringWindowToTop(attached);
//    }

//        ::MoveWindow(hwnd, rect.right - 360, rect.top + 82, 360 - 8,
//        rect.bottom - rect.top - 82 - 8, false);
#endif
}

// void AttachedWindow::nativeEvent(const QByteArray &eventType, void *message,
// long *result)
//{
//    MSG *msg = reinterpret_cast

//    case WM_NCCALCSIZE: {
//    }
//}

std::vector<AttachedWindow::Item> AttachedWindow::items;

}  // namespace chatterino
