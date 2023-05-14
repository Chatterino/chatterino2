#pragma once

#ifdef CHATTERINO_WITH_CRASHPAD
#    include <client/crashpad_client.h>

#    include <memory>

namespace chatterino {

std::unique_ptr<crashpad::CrashpadClient> installCrashHandler();

}  // namespace chatterino

#endif
