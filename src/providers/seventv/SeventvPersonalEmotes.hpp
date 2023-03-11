#pragma once

#include "common/Atomic.hpp"
#include "common/Singleton.hpp"
#include "messages/Emote.hpp"
#include "providers/seventv/eventapi/Dispatch.hpp"

#include <boost/optional.hpp>
#include <pajlada/signals/signalholder.hpp>

#include <memory>
#include <shared_mutex>
#include <unordered_map>

namespace chatterino {

class SeventvPersonalEmotes : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;

    void createEmoteSet(const QString &id);

    // Returns the emote-map of this set if it's new.
    boost::optional<std::shared_ptr<const EmoteMap>> assignUserToEmoteSet(
        const QString &emoteSetID, const QString &userTwitchID);

    void updateEmoteSet(const QString &id,
                        const seventv::eventapi::EmoteAddDispatch &dispatch);
    void updateEmoteSet(const QString &id,
                        const seventv::eventapi::EmoteUpdateDispatch &dispatch);
    void updateEmoteSet(const QString &id,
                        const seventv::eventapi::EmoteRemoveDispatch &dispatch);

    void addEmoteSetForUser(const QString &emoteSetID, EmoteMap &&map,
                            const QString &userTwitchID);

    bool hasEmoteSet(const QString &id) const;

    boost::optional<std::shared_ptr<const EmoteMap>> getEmoteSetForUser(
        const QString &userID) const;

    boost::optional<EmotePtr> getEmoteForUser(const QString &userID,
                                              const EmoteName &emoteName) const;

private:
    // emoteSetID => emoteSet
    std::unordered_map<QString, Atomic<std::shared_ptr<const EmoteMap>>>
        emoteSets_;
    // userID => emoteSetID
    std::unordered_map<QString, QString> userEmoteSets_;

    bool enabled_ = true;
    pajlada::Signals::SignalHolder signalHolder_;

    mutable std::shared_mutex mutex_;
};

}  // namespace chatterino
