#pragma once

#ifdef USEWINSDK

#    include <Windows.h>
#    include <boost/optional.hpp>

namespace ab
{
    boost::optional<UINT> getWindowDpi(HWND hwnd);
    void flushClipboard();

    void moveToCursor(HWND hwnd);

    bool isLButtonDown();
}  // namespace ab

#endif
