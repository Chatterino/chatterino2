#include "util/RenameThread.hpp"

#include "common/QLogging.hpp"

#ifdef Q_OS_WIN

#    include <Windows.h>

namespace chatterino::windows::detail {

void renameThread(HANDLE hThread, const QString &threadName)
{
#    if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // SetThreadDescription requires Windows 10, version 1607
    // Qt 6 requires Windows 10 1809

    auto hr = SetThreadDescription(hThread, threadName.toStdWString().c_str());
    if (!SUCCEEDED(hr))
    {
        qCWarning(chatterinoCommon).nospace()
            << "Failed to set thread description, hresult=0x"
            << QString::number(hr, 16);
    }
#    endif
}

}  // namespace chatterino::windows::detail

#endif
