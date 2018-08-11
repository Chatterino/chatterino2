#pragma once

#include <QString>
#include <messages/Emote.hpp>
#include <unordered_map>
#include "util/QStringHash.hpp"

namespace chatterino {

class Settings;
class Paths;

class TwitchBadges
{
public:
    TwitchBadges();

    void initialize(Settings &settings, Paths &paths);

private:
    void loadTwitchBadges();

    std::unordered_map<QString, EmotePtr> badges;
};

}  // namespace chatterino
