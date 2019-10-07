#pragma once

#ifdef USEWINSDK

#    include <Windows.h>
#    include <boost/optional.hpp>

namespace chatterino {

boost::optional<UINT> getWindowDpi(HWND hwnd);
void flushClipboard();

bool isRegisteredForStartup();
void setRegisteredForStartup(bool isRegistered);

}  // namespace chatterino

#endif
