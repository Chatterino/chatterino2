#pragma once

#ifdef USEWINSDK

#    include <Windows.h>
#    include <boost/optional.hpp>

namespace AB_NAMESPACE {

boost::optional<UINT> getWindowDpi(HWND hwnd);
void flushClipboard();

bool isRegisteredForStartup();
void setRegisteredForStartup(bool isRegistered);

}  // namespace AB_NAMESPACE

#endif
