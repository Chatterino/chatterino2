#include <client/crashpad_client.h>
#include <memory>

namespace chatterino::crasquish {

std::unique_ptr<crashpad::CrashpadClient> installCrashHandler();

}  // namespace chatterino::crasquish
