// SPDX-FileCopyrightText: 2019 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

namespace chatterino {

class Args;

class Modes
{
public:
    struct Init {
        bool isPortable = false;
        bool isExternallyPackaged = false;
    };

    explicit Modes(const Args &args);

    explicit Modes(Init init);

    /// Marked by the line `portable` or `portableEnable` from `Args`
    bool isPortable{};

    /// Marked by the line `externally-packaged`
    ///
    /// The externally packaged mode comes with the following changes:
    ///  - No shortcuts are created by default
    bool isExternallyPackaged{};
};

}  // namespace chatterino
