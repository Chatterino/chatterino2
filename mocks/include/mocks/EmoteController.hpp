#pragma once

#include "controllers/emotes/EmoteController.hpp"
#include "providers/emoji/Emojis.hpp"

namespace chatterino::mock {

class EmoteController : public chatterino::EmoteController
{
public:
    EmoteController()
    {
        this->getEmojis()->load();
    }

    void initialize() override
    {
    }
};

}  // namespace chatterino::mock
