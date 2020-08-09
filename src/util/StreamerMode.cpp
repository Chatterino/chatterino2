#include "StreamerMode.hpp"

#ifdef USEWINSDK
#    include <Windows.h>

#    include <VersionHelpers.h>
#    include <WtsApi32.h>
#    pragma comment(lib, "Wtsapi32.lib")
#endif

namespace chatterino {

const QStringList &broadcastingBinaries()
{
#ifdef USEWINSDK
    static QStringList bins = {"obs.exe", "obs64.exe"};
#else
    static QStringList bins = {};
#endif
    return bins;
}

bool isInStreamerMode()
{
#ifdef USEWINSDK
    if (IsWindowsVistaOrGreater())
    {
        WTS_PROCESS_INFO *pWPIs = nullptr;
        DWORD dwProcCount = 0;

        if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs,
                                  &dwProcCount))
        {
            //Go through all processes retrieved
            for (DWORD i = 0; i < dwProcCount; i++)
            {
                QString processName = QString::fromUtf16(
                    reinterpret_cast<char16_t *>(pWPIs[i].pProcessName));

                if (broadcastingBinaries().contains(processName))
                    return true;
            }
        }

        if (pWPIs)
            WTSFreeMemory(pWPIs);
    }
#endif

    return false;
}

}  // namespace chatterino
