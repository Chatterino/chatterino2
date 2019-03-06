#pragma once

namespace chatterino
{
	// TODO: rename
    const char* version = "2.1.0";

	[[deprecated]] constexpr const char* CHATTERINO_VERSION = version;

#if defined(Q_OS_WIN)
    constexpr const char* CHATTERINO_OS = "win";
#elif defined(Q_OS_MACOS)
	constexpr const char* CHATTERINO_OS = "macos"
#elif defined(Q_OS_LINUX)
	constexpr const char* CHATTERINO_OS = "linux"
#endif
}  // namespace chatterino
