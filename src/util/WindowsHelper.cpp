#include "WindowsHelper.hpp"

#ifdef USEWINSDK

namespace chatterino {

typedef enum MONITOR_DPI_TYPE {
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

typedef HRESULT(CALLBACK *GetDpiForMonitor_)(HMONITOR, MONITOR_DPI_TYPE, UINT *,
                                             UINT *);

boost::optional<UINT> getWindowDpi(HWND hwnd)
{
    static HINSTANCE shcore = LoadLibrary(L"Shcore.dll");
    if (shcore != nullptr) {
        if (auto getDpiForMonitor =
                GetDpiForMonitor_(GetProcAddress(shcore, "GetDpiForMonitor"))) {
            HMONITOR monitor =
                MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

            UINT xScale, yScale;

            getDpiForMonitor(monitor, MDT_DEFAULT, &xScale, &yScale);

            return xScale;
        }
    }

    return boost::none;
}

typedef HRESULT(CALLBACK *OleFlushClipboard_)();

void flushClipboard()
{
    static HINSTANCE ole32 = LoadLibrary(L"Ole32.dll");
    if (ole32 != nullptr) {
        if (auto oleFlushClipboard = OleFlushClipboard_(GetProcAddress(ole32, "OleFlushClipboard"))) {
            oleFlushClipboard();
        }
    }
}

}  // namespace chatterino

#endif
