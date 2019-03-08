#if 0
#    include "AttachedWindow.hpp"

#    include "Application.hpp"
#    include "util/DebugCount.hpp"
#    include "widgets/splits/Split.hpp"

#    include <QTimer>
#    include <QVBoxLayout>

#    ifdef USEWINSDK
#        include "util/WindowsHelper.hpp"

#        include "Windows.h"
// don't even think about reordering these
#        include "Psapi.h"
#        pragma comment(lib, "Dwmapi.lib")
#    endif

namespace chatterino
{
    AttachedWindow::AttachedWindow(void* _target, int _yOffset)
        : QWidget(nullptr, Qt::FramelessWindowHint | Qt::Window)
        , target_(_target)
        , yOffset_(_yOffset)
    {
        QLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        this->setLayout(layout);

        auto* split = new Split(this);
        this->ui_.split = split;
        split->setSizePolicy(
            QSizePolicy::Maximum, QSizePolicy::MinimumExpanding);
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

    AttachedWindow* AttachedWindow::get(void* target, const GetArgs& args)
    {
        AttachedWindow* window = [&]() {
            for (Item& item : items)
            {
                if (item.hwnd == target)
                {
                    return item.window;
                }
            }

            auto* window = new AttachedWindow(target, args.yOffset);
            items.push_back(Item{target, window, args.winId});
            return window;
        }();

        bool show = true;
        QSize size = window->size();

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

    void AttachedWindow::detach(const QString& winId)
    {
        for (Item& item : items)
        {
            if (item.winId == winId)
            {
                item.window->deleteLater();
            }
        }
    }

    void AttachedWindow::setChannel(ChannelPtr channel)
    {
        this->ui_.split->setChannel(channel);
    }

    void AttachedWindow::showEvent(QShowEvent*)
    {
        this->attachToHwnd(this->target_);
    }

    void AttachedWindow::attachToHwnd(void* _attachedPtr)
    {
#    ifdef USEWINSDK
        if (this->attached_)
        {
            return;
        }

        this->attached_ = true;
        this->timer_.setInterval(1);

        auto hwnd = HWND(this->winId());
        auto attached = HWND(_attachedPtr);

        QObject::connect(
            &this->timer_, &QTimer::timeout, [this, hwnd, attached] {
                // check process id
                if (!this->validProcessName_)
                {
                    DWORD processId;
                    ::GetWindowThreadProcessId(attached, &processId);

                    HANDLE process = ::OpenProcess(
                        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false,
                        processId);

                    std::unique_ptr<TCHAR[]> filename(new TCHAR[512]);
                    DWORD filenameLength = ::GetModuleFileNameEx(
                        process, nullptr, filename.get(), 512);
                    QString qfilename =
                        QString::fromWCharArray(filename.get(), filenameLength);

                    if (!qfilename.endsWith("chrome.exe") &&
                        !qfilename.endsWith("firefox.exe"))
                    {
                        qDebug() << "NM Illegal caller" << qfilename;
                        this->timer_.stop();
                        this->deleteLater();
                        return;
                    }
                    this->validProcessName_ = true;
                }

                this->updateWindowRect(attached);
            });
        this->timer_.start();
#    endif
    }

    void AttachedWindow::updateWindowRect(void* _attachedPtr)
    {
#    ifdef USEWINSDK
        auto hwnd = HWND(this->winId());
        auto attached = HWND(_attachedPtr);

        // We get the window rect first so we can close this window when it
        // returns an error. If we query the process first and check the
        // filename then it will return and empty string that doens't match.
        ::SetLastError(0);
        RECT rect;
        ::GetWindowRect(attached, &rect);

        if (::GetLastError() != 0)
        {
            qDebug() << "NM GetLastError()" << ::GetLastError();

            this->timer_.stop();
            this->deleteLater();
            return;
        }

        // set the correct z-order
        HWND next = ::GetNextWindow(attached, GW_HWNDPREV);

        ::SetWindowPos(hwnd, next ? next : HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        float scale = 1.f;
        if (auto dpi = getWindowDpi(attached))
        {
            scale = dpi.get() / 96.f;

            //        for (auto w : this->ui_.split->findChildren<ab::BaseWidget
            //        *>()) {
            //            w->setOverrideScale(scale);
            //        }
            //        this->ui_.split->setOverrideScale(scale);
        }

        if (this->height_ == -1)
        {
            // ::MoveWindow(hwnd, rect.right - this->width_ - 8, rect.top +
            // this->yOffset_ - 8,
            //              this->width_, rect.bottom - rect.top -
            //              this->yOffset_, false);
        }
        else
        {
            ::MoveWindow(hwnd,                                 //
                int(rect.right - this->width_ * scale - 8),    //
                int(rect.bottom - this->height_ * scale - 8),  //
                int(this->width_ * scale), int(this->height_ * scale), true);
        }

//        ::MoveWindow(hwnd, rect.right - 360, rect.top + 82, 360 - 8,
//        rect.bottom - rect.top - 82 - 8, false);
#    endif
    }

    // void AttachedWindow::nativeEvent(const QByteArray &eventType, void
    // *message, long *result)
    //{
    //    MSG *msg = reinterpret_cast

    //    case WM_NCCALCSIZE: {
    //    }
    //}

    std::vector<AttachedWindow::Item> AttachedWindow::items;

}  // namespace chatterino
#endif
