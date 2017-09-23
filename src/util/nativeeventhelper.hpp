#pragma once

#ifdef USEWINSDK
#include <windows.h>
#include <boost/optional.hpp>

#include <QLibrary>

namespace chatterino {
namespace util {
static bool tryHandleDpiChangedMessage(void *message, int &dpi)
{
    MSG *msg = reinterpret_cast<MSG *>(message);

    // WM_DPICHANGED
    if (msg->message == 0x02E0) {
        dpi = HIWORD(msg->wParam);

        return true;
    }
    return false;
}

static boost::optional<UINT> getWindowDpi(quintptr ptr)
{
    typedef UINT(WINAPI * GetDpiForWindow)(HWND);
    QLibrary user32("user32.dll", NULL);

    GetDpiForWindow getDpiForWindow = (GetDpiForWindow)user32.resolve("GetDpiForWindow");

    if (getDpiForWindow) {
        UINT value = getDpiForWindow((HWND)ptr);

        return value == 0 ? boost::none : boost::optional<UINT>(value);
    }

    return boost::none;
}
}  // namespace util
}  // namespace chatterino

#endif
