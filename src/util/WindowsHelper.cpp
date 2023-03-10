#include "WindowsHelper.hpp"

#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>

#ifdef USEWINSDK

#    include <Shlwapi.h>
#    include <VersionHelpers.h>

namespace chatterino {

typedef enum MONITOR_DPI_TYPE {
    MDT_EFFECTIVE_DPI = 0,
    MDT_ANGULAR_DPI = 1,
    MDT_RAW_DPI = 2,
    MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

typedef HRESULT(CALLBACK *GetDpiForMonitor_)(HMONITOR, MONITOR_DPI_TYPE, UINT *,
                                             UINT *);
typedef HRESULT(CALLBACK *AssocQueryString_)(ASSOCF, ASSOCSTR, LPCWSTR, LPCWSTR,
                                             LPWSTR, DWORD *);

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

QString getAssociatedCommand(AssociationQueryType queryType, LPCWSTR query)
{
    static HINSTANCE shlwapi = LoadLibrary(L"shlwapi");
    if (shlwapi == nullptr)
    {
        return QString();
    }

    static auto assocQueryString =
        AssocQueryString_(GetProcAddress(shlwapi, "AssocQueryStringW"));
    if (assocQueryString == nullptr)
    {
        return QString();
    }

    // always error out instead of returning a truncated string when the
    // buffer is too small - avoids race condition when the user changes their
    // default browser between calls to AssocQueryString
    ASSOCF flags = ASSOCF_NOTRUNCATE;

    if (queryType == AssociationQueryType::Protocol)
    {
        // ASSOCF_IS_PROTOCOL was introduced in Windows 8
        if (IsWindows8OrGreater())
        {
            flags |= ASSOCF_IS_PROTOCOL;
        }
        else
        {
            return QString();
        }
    }

    DWORD resultSize = 0;
    if (FAILED(assocQueryString(flags, ASSOCSTR_COMMAND, query, nullptr,
                                nullptr, &resultSize)))
    {
        return QString();
    }

    if (resultSize <= 1)
    {
        // resultSize includes the null terminator. if resultSize is 1, the
        // returned value would be the empty string.
        return QString();
    }

    QString result;
    auto buf = new wchar_t[resultSize];
    if (SUCCEEDED(assocQueryString(flags, ASSOCSTR_COMMAND, query, nullptr, buf,
                                   &resultSize)))
    {
        // QString::fromWCharArray expects the length in characters *not
        // including* the null terminator, but AssocQueryStringW calculates
        // length including the null terminator
        result = QString::fromWCharArray(buf, resultSize - 1);
    }
    delete[] buf;
    return result;
}

}  // namespace chatterino

#endif
