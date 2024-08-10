#include "util/RenameThread.hpp"

#ifdef Q_OS_WIN
#    include <Windows.h>
#endif

namespace chatterino::windows::detail {

void renameThread(HANDLE hThread, const QString &threadName)
{
    SetThreadDescription(hThread, threadName.toStdWString().c_str());
}

}  // namespace chatterino::windows::detail
