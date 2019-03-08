#pragma once

namespace chatterino
{
    // TODO: rename
    [[deprecated]] constexpr const char* version = "2.0.4";

    [[maybe_unused]] constexpr const char* CHATTERINO_VERSION = version;

#if defined(Q_OS_WIN)
    [[maybe_unused]] constexpr const char* CHATTERINO_OS = "win";
#elif defined(Q_OS_MACOS)
    [[maybe_unused]] constexpr const char* CHATTERINO_OS = "macos"
#elif defined(Q_OS_LINUX)
    [[maybe_unused]] constexpr const char* CHATTERINO_OS = "linux"
#endif
}  // namespace chatterino
