#include "util/AttachToConsole.hpp"

#ifdef USEWINSDK
#    include <Windows.h>

#    include <cstdio>
#    include <tuple>
#endif

namespace chatterino {

void attachToConsole()
{
#ifdef USEWINSDK
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        std::ignore = freopen_s(nullptr, "CONOUT$", "w", stdout);
        std::ignore = freopen_s(nullptr, "CONOUT$", "w", stderr);
    }
#endif
}

}  // namespace chatterino
