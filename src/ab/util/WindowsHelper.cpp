#include "WindowsHelper.hpp"

#ifdef USEWINSDK

namespace ab
{
    typedef enum MONITOR_DPI_TYPE {
        MDT_EFFECTIVE_DPI = 0,
        MDT_ANGULAR_DPI = 1,
        MDT_RAW_DPI = 2,
        MDT_DEFAULT = MDT_EFFECTIVE_DPI
    } MONITOR_DPI_TYPE;

    typedef HRESULT(CALLBACK* GetDpiForMonitor_)(
        HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

    std::optional<UINT> getWindowDpi(HWND hwnd)
    {
        SetProcessDpiAwarenessContext(
            DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        static HINSTANCE shcore = LoadLibrary(L"Shcore.dll");
        if (shcore != nullptr)
        {
            if (auto getDpiForMonitor = GetDpiForMonitor_(
                    GetProcAddress(shcore, "GetDpiForMonitor")))
            {
                HMONITOR monitor =
                    MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

                UINT xScale, yScale;

                getDpiForMonitor(monitor, MDT_DEFAULT, &xScale, &yScale);

                return xScale;
            }
        }

        return std::nullopt;
    }

    typedef HRESULT(CALLBACK* OleFlushClipboard_)();

    void flushClipboard()
    {
        static HINSTANCE ole32 = LoadLibrary(L"Ole32.dll");
        if (ole32 != nullptr)
        {
            if (auto oleFlushClipboard = OleFlushClipboard_(
                    GetProcAddress(ole32, "OleFlushClipboard")))
            {
                oleFlushClipboard();
            }
        }
    }

    void moveToCursor(HWND hwnd)
    {
        POINT point;
        GetCursorPos(&point);
        SetWindowPos(hwnd, nullptr, point.x - 40, point.y - 10, 500, 200, 0);
    }

    bool isLButtonDown()
    {
        return GetKeyState(VK_LBUTTON) & 0x100;
    }
}  // namespace ab

#endif
