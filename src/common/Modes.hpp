#pragma once

namespace chatterino {

class Modes
{
public:
    Modes();

    static const Modes &getInstance();

    bool isNightly{};
    bool isPortable{};
};

}  // namespace chatterino
