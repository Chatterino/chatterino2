#pragma once

namespace chatterino
{
    [[maybe_unused]] constexpr const char* CHATTERINO_VERSION = "2.0.4";

#if defined(Q_OS_WIN)
    [[maybe_unused]] constexpr const char* CHATTERINO_OS = "win";
#elif defined(Q_OS_MACOS)
    [[maybe_unused]] constexpr const char* CHATTERINO_OS = "macos"
#elif defined(Q_OS_LINUX)
    [[maybe_unused]] constexpr const char* CHATTERINO_OS = "linux"
#endif
}  // namespace chatterino
