#include "providers/seventv/SeventvPersonalEmotes.hpp"

#include "providers/seventv/SeventvEmotes.hpp"
#include "singletons/Settings.hpp"

#include <boost/optional/optional.hpp>

#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace chatterino {

void SeventvPersonalEmotes::initialize(Settings &settings, Paths & /*paths*/)
{
    settings.enableSevenTVPersonalEmotes.connect(
        [this]() {
            std::unique_lock<std::shared_mutex> lock(this->mutex_);
            this->enabled_ = Settings::instance().enableSevenTVPersonalEmotes;
        },
        this->signalHolder_);
}

void SeventvPersonalEmotes::createEmoteSet(const QString &id)
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
    if (!this->emoteSets_.contains(id))
    {
        this->emoteSets_.emplace(id, std::make_shared<const EmoteMap>());
    }
}

boost::optional<std::shared_ptr<const EmoteMap>>
    SeventvPersonalEmotes::assignUserToEmoteSet(const QString &emoteSetID,
                                                const QString &userTwitchID)
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
    if (!this->userEmoteSets_.contains(userTwitchID))
    {
        this->userEmoteSets_.emplace(userTwitchID, emoteSetID);

        auto set = this->emoteSets_.find(emoteSetID);
        if (set == this->emoteSets_.end())
        {
            return boost::none;
        }
        return set->second.get();  // copy the shared_ptr
    }
    return boost::none;
}

void SeventvPersonalEmotes::updateEmoteSet(
    const QString &id, const seventv::eventapi::EmoteAddDispatch &dispatch)
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
    auto emoteSet = this->emoteSets_.find(id);
    if (emoteSet != this->emoteSets_.end())
    {
        // Make sure this emote is actually new to avoid copying the map
        if (emoteSet->second.get()->contains(
                EmoteName{dispatch.emoteJson["name"].toString()}))
        {
            return;
        }
        SeventvEmotes::addEmote(emoteSet->second, dispatch,
                                SeventvEmoteSetKind::Personal);
    }
}
void SeventvPersonalEmotes::updateEmoteSet(
    const QString &id, const seventv::eventapi::EmoteUpdateDispatch &dispatch)
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
    auto emoteSet = this->emoteSets_.find(id);
    if (emoteSet != this->emoteSets_.end())
    {
        SeventvEmotes::updateEmote(emoteSet->second, dispatch,
                                   SeventvEmoteSetKind::Personal);
    }
}
void SeventvPersonalEmotes::updateEmoteSet(
    const QString &id, const seventv::eventapi::EmoteRemoveDispatch &dispatch)
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
    auto emoteSet = this->emoteSets_.find(id);
    if (emoteSet != this->emoteSets_.end())
    {
        SeventvEmotes::removeEmote(emoteSet->second, dispatch);
    }
}

void SeventvPersonalEmotes::addEmoteSetForUser(const QString &emoteSetID,
                                               EmoteMap &&map,
                                               const QString &userTwitchID)
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
    this->emoteSets_.emplace(emoteSetID, std::make_shared<const EmoteMap>(map));
    this->userEmoteSets_[userTwitchID] = emoteSetID;
}

bool SeventvPersonalEmotes::hasEmoteSet(const QString &id) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    return this->emoteSets_.contains(id);
}

boost::optional<std::shared_ptr<const EmoteMap>>
    SeventvPersonalEmotes::getEmoteSetForUser(const QString &userID) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (!this->enabled_)
    {
        return boost::none;
    }

    auto id = this->userEmoteSets_.find(userID);
    if (id == this->userEmoteSets_.end())
    {
        return boost::none;
    }
    auto set = this->emoteSets_.find(id->second);
    if (set == this->emoteSets_.end())
    {
        return boost::none;
    }
    return set->second.get();  // copy the shared_ptr
}

boost::optional<EmotePtr> SeventvPersonalEmotes::getEmoteForUser(
    const QString &userID, const EmoteName &emoteName) const
{
    return this->getEmoteSetForUser(userID).flat_map(
        [emoteName](const auto &map) -> boost::optional<EmotePtr> {
            auto it = map->find(emoteName);
            if (it == map->end())
            {
                return boost::none;
            }
            return it->second;
        });
}

}  // namespace chatterino
