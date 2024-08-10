#include "util/RenameThread.hpp"

#include "common/QLogging.hpp"

#ifdef Q_OS_WIN
#    include <Windows.h>
#endif

namespace chatterino::windows::detail {

void renameThread(HANDLE hThread, const QString &threadName)
{
    auto hr = SetThreadDescription(hThread, threadName.toStdWString().c_str());
    if (!SUCCEEDED(hr))
    {
        qCWarning(chatterinoCommon).nospace()
            << "Failed to set thread description, hresult=0x"
            << QString::number(hr, 16);
    }
}

}  // namespace chatterino::windows::detail
