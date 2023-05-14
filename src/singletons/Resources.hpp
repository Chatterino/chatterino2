#pragma once

#include "ResourcesAutogen.hpp"

namespace chatterino {

/// This class in thread safe but needs to be initialized from the gui thread
/// first.
Resources2 &getResources();
void initResources();

}  // namespace chatterino
