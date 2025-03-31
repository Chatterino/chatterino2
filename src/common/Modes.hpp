#pragma once

namespace chatterino {

class Modes
{
public:
    Modes();

    static const Modes &instance();

    bool isNightly{};
    bool isPortable{};
};

}  // namespace chatterino
