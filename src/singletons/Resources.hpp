// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "ResourcesAutogen.hpp"

namespace chatterino {

/// This class in thread safe but needs to be initialized from the gui thread
/// first.
Resources2 &getResources();
void initResources();

}  // namespace chatterino
