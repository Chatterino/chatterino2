#pragma once

#ifdef USEWINSDK

#    include <Windows.h>
#    include <optional>

namespace ab
{
    std::optional<UINT> getWindowDpi(HWND hwnd);
    void flushClipboard();

    void moveToCursor(HWND hwnd);

    bool isLButtonDown();
}  // namespace ab

#endif
