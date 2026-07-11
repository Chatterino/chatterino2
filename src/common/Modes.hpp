// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

namespace chatterino {

class Args;

class Modes
{
public:
    explicit Modes(const Args &args);

    /// Marked by the line `portable` or `--portable[-dir]` option from `Args`
    bool isPortable{};

    /// Marked by the line `externally-packaged`
    ///
    /// The externally packaged mode comes with the following changes:
    ///  - No shortcuts are created by default
    bool isExternallyPackaged{};
};

}  // namespace chatterino
