#pragma once

#include <string>
#include <vector>

/// Parse an encoded command line string from chatterino into its arguments.
///
/// The command line arguments are joined by '+'. A plus is escaped by an
/// additional plus ('++' -> '+').
///
/// The encoding happens in singletons/CrashHandler.cpp
std::vector<std::wstring> splitEncodedChatterinoArgs(
    const std::wstring &encodedArgs);
