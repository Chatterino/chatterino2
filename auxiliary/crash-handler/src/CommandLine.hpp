#pragma once

#include <string>
#include <vector>

/// Parse a command line string from chatterino into its arguments.
///
/// The command line arguments are joined by '+'. A plus is escaped by an
/// additional plus ('++' -> '+').
std::vector<std::wstring> splitChatterinoArgs(const std::wstring &args);
