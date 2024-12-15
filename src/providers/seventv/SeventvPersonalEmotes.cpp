#include "providers/seventv/SeventvPersonalEmotes.hpp"

#include "common/Literals.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "singletons/Settings.hpp"
#include "util/DebugCount.hpp"

#include <memory>
#include <mutex>
#include <optional>

namespace chatterino {

using namespace literals;

SeventvPersonalEmotes::SeventvPersonalEmotes()
{
    getSettings()->enableSevenTVPersonalEmotes.connect(
        [this]() {
            std::unique_lock<std::shared_mutex> lock(this->mutex_);
            this->enabled_ = getSettings()->enableSevenTVPersonalEmotes;
        },
        this->signalHolder_);
}

void SeventvPersonalEmotes::createEmoteSet(const QString &id)
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);
    if (!this->emoteSets_.contains(id))
    {
        DebugCount::increase(u"7TV Personal Emote Sets"_s);
        this->emoteSets_.emplace(id, std::make_shared<const EmoteMap>());
    }
}

std::optional<std::shared_ptr<const EmoteMap>>
    SeventvPersonalEmotes::assignUserToEmoteSet(const QString &emoteSetID,
                                                const QString &userTwitchID)
{
    std::unique_lock<std::shared_mutex> lock(this->mutex_);

    auto &list = this->userEmoteSets_[userTwitchID];

    if (list.contains(emoteSetID))
    {
        return std::nullopt;
    }
    list.append(emoteSetID);
    DebugCount::increase(u"7TV Personal Emote Assignments"_s);

    auto set = this->emoteSets_.find(emoteSetID);
    if (set == this->emoteSets_.end())
    {
        return std::nullopt;
    }
    return set->second.get();  // copy the shared_ptr
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
    bool added = this->emoteSets_
                     .emplace(emoteSetID,
                              std::make_shared<const EmoteMap>(std::move(map)))
                     .second;
    if (added)
    {
        DebugCount::increase(u"7TV Personal Emote Sets"_s);
    }
    this->userEmoteSets_[userTwitchID].append(emoteSetID);
    DebugCount::increase(u"7TV Personal Emote Assignments"_s);
}

bool SeventvPersonalEmotes::hasEmoteSet(const QString &id) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    return this->emoteSets_.contains(id);
}

QList<std::shared_ptr<const EmoteMap>>
    SeventvPersonalEmotes::getEmoteSetsForUser(const QString &userID) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (!this->enabled_)
    {
        return {};
    }

    auto ids = this->userEmoteSets_.find(userID);
    if (ids == this->userEmoteSets_.end())
    {
        return {};
    }

    QList<std::shared_ptr<const EmoteMap>> sets;
    sets.reserve(ids->second.length());
    for (const auto &id : ids->second)
    {
        auto set = this->emoteSets_.find(id);
        if (set == this->emoteSets_.end())
        {
            continue;
        }
        sets.append(set->second.get());  // copy the shared_ptr
    }

    return sets;
}

std::optional<EmotePtr> SeventvPersonalEmotes::getEmoteForUser(
    const QString &userID, const EmoteName &emoteName) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (!this->enabled_)
    {
        return {};
    }

    auto ids = this->userEmoteSets_.find(userID);
    if (ids == this->userEmoteSets_.end())
    {
        return {};
    }

    for (const auto &id : ids->second)
    {
        auto setIt = this->emoteSets_.find(id);
        if (setIt == this->emoteSets_.end())
        {
            continue;  // set doesn't exist
        }

        const auto &set = setIt->second.get();
        auto it = set->find(emoteName);
        if (it == set->end())
        {
            continue;  // not in this set
        }
        return it->second;  // found the emote
    }

    return std::nullopt;
}

std::optional<std::shared_ptr<const EmoteMap>>
    SeventvPersonalEmotes::getEmoteSetByID(const QString &emoteSetID) const
{
    std::shared_lock<std::shared_mutex> lock(this->mutex_);
    if (!this->enabled_)
    {
        return std::nullopt;
    }

    auto id = this->emoteSets_.find(emoteSetID);
    if (id == this->emoteSets_.end())
    {
        return std::nullopt;
    }
    return id->second.get();
}

}  // namespace chatterino
