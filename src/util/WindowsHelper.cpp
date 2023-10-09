#include "util/WindowsHelper.hpp"

#include "common/Literals.hpp"

#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QSettings>

#ifdef USEWINSDK

#    include <Ole2.h>
#    include <ShellScalingApi.h>
#    include <Shlwapi.h>
#    include <VersionHelpers.h>

namespace chatterino {

using namespace literals;

using GetDpiForMonitor_ = HRESULT(CALLBACK *)(HMONITOR, MONITOR_DPI_TYPE,
                                              UINT *, UINT *);

// TODO: This should be changed to `GetDpiForWindow`.
std::optional<UINT> getWindowDpi(HWND hwnd)
{
    static HINSTANCE shcore = LoadLibrary(L"Shcore.dll");
    if (shcore != nullptr)
    {
        if (auto getDpiForMonitor =
                GetDpiForMonitor_(GetProcAddress(shcore, "GetDpiForMonitor")))
        {
            HMONITOR monitor =
                MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

            UINT xScale = 96;
            UINT yScale = 96;
            getDpiForMonitor(monitor, MDT_DEFAULT, &xScale, &yScale);

            return xScale;
        }
    }

    return std::nullopt;
}

void flushClipboard()
{
    if (QApplication::clipboard()->ownsClipboard())
    {
        OleFlushClipboard();
    }
}

const QString RUN_KEY =
    uR"(HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run)"_s;

bool isRegisteredForStartup()
{
    QSettings settings(RUN_KEY, QSettings::NativeFormat);

    return !settings.value("Chatterino").toString().isEmpty();
}

void setRegisteredForStartup(bool isRegistered)
{
    QSettings settings(RUN_KEY, QSettings::NativeFormat);

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

QString getAssociatedExecutable(AssociationQueryType queryType, LPCWSTR query)
{
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
            return {};
        }
    }

    DWORD resultSize = 0;
    if (FAILED(AssocQueryStringW(flags, ASSOCSTR_EXECUTABLE, query, nullptr,
                                 nullptr, &resultSize)))
    {
        return {};
    }

    if (resultSize <= 1)
    {
        // resultSize includes the null terminator. if resultSize is 1, the
        // returned value would be the empty string.
        return {};
    }

    QString result;
    auto *buf = new wchar_t[resultSize];
    if (SUCCEEDED(AssocQueryStringW(flags, ASSOCSTR_EXECUTABLE, query, nullptr,
                                    buf, &resultSize)))
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
