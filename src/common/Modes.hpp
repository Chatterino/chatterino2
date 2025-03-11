#pragma once

namespace chatterino {

class Modes
{
public:
    Modes();

    static const Modes &instance();

    bool isNightly{};
    bool isPortable{};

    /// Marked by the line `externally-packaged`
    ///
    /// The externally packaged mode comes with the following changes:
    ///  - No shortcuts are created by default
    bool isExternallyPackaged{};
};

}  // namespace chatterino
