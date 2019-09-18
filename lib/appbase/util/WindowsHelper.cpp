#include "WindowsHelper.hpp"

#include <QSettings>

#ifdef USEWINSDK

namespace AB_NAMESPACE {

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
    if (shcore != nullptr)
    {
        if (auto getDpiForMonitor =
                GetDpiForMonitor_(GetProcAddress(shcore, "GetDpiForMonitor")))
        {
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
    if (ole32 != nullptr)
    {
        if (auto oleFlushClipboard =
                OleFlushClipboard_(GetProcAddress(ole32, "OleFlushClipboard")))
        {
            oleFlushClipboard();
        }
    }
}

constexpr const char *runKey =
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool isRegisteredForStartup()
{
    QSettings settings(runKey, QSettings::NativeFormat);

    return !settings.value("Chatterino").toString().isEmpty();
}

void setRegisteredForStartup(bool isRegistered)
{
    QSettings settings(runKey, QSettings::NativeFormat);

    if (isRegistered)
    {
        auto exePath = QFileInfo(QCoreApplication::applicationFilePath())
                           .absoluteFilePath()
                           .replace('/', '\\');

        settings.setValue("Chatterino", "\"" + exePath + "\" --autorun");
    }
    else
    {
        settings.remove("Chatterino");
    }
}

}  // namespace AB_NAMESPACE

#endif
