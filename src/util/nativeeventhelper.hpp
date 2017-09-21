#pragma once

#ifdef USEWINSDK
#include "windows.h"

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
}
}

#endif
