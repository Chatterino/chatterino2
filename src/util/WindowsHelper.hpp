#pragma once

#ifdef USEWINSDK

#    include <boost/optional.hpp>
#    include <Windows.h>

namespace chatterino {

boost::optional<UINT> getWindowDpi(HWND hwnd);
void flushClipboard();

bool isRegisteredForStartup();
void setRegisteredForStartup(bool isRegistered);

}  // namespace chatterino

#endif
