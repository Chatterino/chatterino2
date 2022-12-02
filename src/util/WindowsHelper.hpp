#pragma once

#ifdef USEWINSDK

// clang-format off
#    include <Windows.h>
#    include <boost/optional.hpp>
// clang-format on

namespace chatterino {

boost::optional<UINT> getWindowDpi(HWND hwnd);
void flushClipboard();

bool isRegisteredForStartup();
void setRegisteredForStartup(bool isRegistered);

}  // namespace chatterino

#endif
