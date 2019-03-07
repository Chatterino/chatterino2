#pragma once

#include <QString>
#include <boost/optional.hpp>
#include <unordered_map>

#include "util/QStringHash.hpp"
#include "util/UniqueAccess.hpp"

namespace chatterino
{
    struct Emote;
    using EmotePtr = std::shared_ptr<const Emote>;

    class Settings;
    class Paths;

    class TwitchBadges
    {
    public:
        void load();

        boost::optional<EmotePtr> badge(
            const QString& set, const QString& version) const;

    private:
        UniqueAccess<
            std::unordered_map<QString, std::unordered_map<QString, EmotePtr>>>
            badgeSets_;  // "bits": { "100": ... "500": ...
    };

    TwitchBadges& twitchBadges();
}  // namespace chatterino
