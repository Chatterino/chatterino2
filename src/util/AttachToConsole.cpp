#include "AttachToConsole.hpp"

#ifdef USEWINSDK
#    include <Windows.h>
#endif

namespace chatterino {

void attachToConsole()
{
#ifdef USEWINSDK
    if (AttachConsole(ATTACH_PARENT_PROCESS))
    {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif
}

}  // namespace chatterino
