#pragma once

#include "controllers/emotes/EmoteController.hpp"
#include "providers/emoji/Emojis.hpp"

namespace chatterino::mock {

class EmoteController : public chatterino::EmoteController
{
public:
    EmoteController()
    {
        this->emojis()->load();
    }

    void addProvider(EmoteProviderPtr provider)
    {
        this->providers_.emplace_back(std::move(provider));
        this->sort();
    }

    void initialize() override
    {
    }
};

}  // namespace chatterino::mock
