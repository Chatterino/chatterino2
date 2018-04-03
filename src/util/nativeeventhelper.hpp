#pragma once

#ifdef USEWINSDK
#include <windows.h>
#include <boost/optional.hpp>

#include <QAbstractNativeEventFilter>
#include <QLibrary>

namespace chatterino {
namespace util {

static boost::optional<UINT> getWindowDpi(quintptr ptr)
{
    typedef UINT(WINAPI * GetDpiForWindow)(HWND);
    QLibrary user32("user32.dll", 0);

    GetDpiForWindow getDpiForWindow = (GetDpiForWindow)user32.resolve("GetDpiForWindow");

    if (getDpiForWindow) {
        UINT value = getDpiForWindow((HWND)ptr);

        return value == 0 ? boost::none : boost::optional<UINT>(value);
    }

    return boost::none;
}

#ifdef USEWINSDK
class DpiNativeEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override
    {
        //        MSG *msg = reinterpret_cast<MSG *>(message);

        //        if (msg->message == WM_NCCREATE) {
        //            QLibrary user32("user32.dll", 0);
        //            {
        //                typedef BOOL(WINAPI * EnableNonClientDpiScaling)(HWND);

        //                EnableNonClientDpiScaling enableNonClientDpiScaling =
        //                    (EnableNonClientDpiScaling)user32.resolve("EnableNonClientDpiScaling");

        //                if (enableNonClientDpiScaling)
        //                    enableNonClientDpiScaling(msg->hwnd);
        //            }
        //        }
        return false;
    }
};
#endif

}  // namespace util
}  // namespace chatterino

#endif
