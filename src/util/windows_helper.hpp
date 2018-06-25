#pragma once

#ifdef USEWINSDK

#include <Windows.h>
#include <boost/optional.hpp>

namespace chatterino {
namespace util {

boost::optional<UINT> getWindowDpi(HWND hwnd);

}  // namespace util
}  // namespace chatterino

#endif
