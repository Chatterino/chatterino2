#pragma once

#ifdef USEWINSDK

#include <Windows.h>
#include <boost/optional.hpp>

namespace chatterino {

boost::optional<UINT> getWindowDpi(HWND hwnd);

}  // namespace chatterino

#endif
