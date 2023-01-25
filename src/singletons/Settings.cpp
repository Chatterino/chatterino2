#include "singletons/Settings.hpp"

#include "controllers/filters/FilterRecord.hpp"
#include "controllers/highlights/HighlightBadge.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "controllers/nicknames/Nickname.hpp"
#include "util/PersistSignalVector.hpp"
#include "util/WindowsHelper.hpp"

namespace chatterino {

ConcurrentSettings *concurrentInstance_{};

ConcurrentSettings::ConcurrentSettings()
    // NOTE: these do not get deleted
    : highlightedMessages(*new SignalVector<HighlightPhrase>())
    , highlightedUsers(*new SignalVector<HighlightPhrase>())
    , highlightedBadges(*new SignalVector<HighlightBadge>())
    , blacklistedUsers(*new SignalVector<HighlightBlacklistUser>())
    , ignoredMessages(*new SignalVector<IgnorePhrase>())
    , mutedChannels(*new SignalVector<QString>())
    , filterRecords(*new SignalVector<FilterRecordPtr>())
    , nicknames(*new SignalVector<Nickname>())
    , moderationActions(*new SignalVector<ModerationAction>)
    , loggedChannels(*new SignalVector<ChannelLog>)
{
    persist(this->highlightedMessages, "/highlighting/highlights");
    persist(this->blacklistedUsers, "/highlighting/blacklist");
    persist(this->highlightedBadges, "/highlighting/badges");
    persist(this->highlightedUsers, "/highlighting/users");
    persist(this->ignoredMessages, "/ignore/phrases");
    persist(this->mutedChannels, "/pings/muted");
    persist(this->filterRecords, "/filtering/filters");
    persist(this->nicknames, "/nicknames");
    // tagged users?
    persist(this->moderationActions, "/moderation/actions");
    persist(this->loggedChannels, "/logging/channels");
}

bool ConcurrentSettings::isHighlightedUser(const QString &username)
{
    auto items = this->highlightedUsers.readOnly();

    for (const auto &highlightedUser : *items)
    {
        if (highlightedUser.isMatch(username))
            return true;
    }

    return false;
}

bool ConcurrentSettings::isBlacklistedUser(const QString &username)
{
    auto items = this->blacklistedUsers.readOnly();

    for (const auto &blacklistedUser : *items)
    {
        if (blacklistedUser.isMatch(username))
            return true;
    }

    return false;
}

bool ConcurrentSettings::isMutedChannel(const QString &channelName)
{
    auto items = this->mutedChannels.readOnly();

    for (const auto &channel : *items)
    {
        if (channelName.toLower() == channel.toLower())
        {
            return true;
        }
    }
    return false;
}

void ConcurrentSettings::mute(const QString &channelName)
{
    mutedChannels.append(channelName);
}

void ConcurrentSettings::unmute(const QString &channelName)
{
    for (std::vector<int>::size_type i = 0; i != mutedChannels.raw().size();
         i++)
    {
        if (mutedChannels.raw()[i].toLower() == channelName.toLower())
        {
            mutedChannels.removeAt(i);
            i--;
        }
    }
}

bool ConcurrentSettings::toggleMutedChannel(const QString &channelName)
{
    if (this->isMutedChannel(channelName))
    {
        unmute(channelName);
        return false;
    }
    else
    {
        mute(channelName);
        return true;
    }
}

ConcurrentSettings &getCSettings()
{
    // `concurrentInstance_` gets assigned in Settings ctor.
    assert(concurrentInstance_);

    return *concurrentInstance_;
}

Settings *Settings::instance_ = nullptr;

Settings::Settings(const QString &settingsDirectory)
    : ABSettings(settingsDirectory)
{
    instance_ = this;
    concurrentInstance_ = this;
    this->moveLegacyDankerinoSettings_();

#ifdef USEWINSDK
    this->autorun = isRegisteredForStartup();
    this->autorun.connect(
        [](bool autorun) {
            setRegisteredForStartup(autorun);
        },
        false);
#endif
}
void Settings::moveLegacyDankerinoSettings_()
{
    if (this->legacyDankerinoRemoveSpacesBetweenEmotes_)
    {
        this->legacyDankerinoRemoveSpacesBetweenEmotes_ = false;
        this->removeSpacesBetweenEmotes = true;
    }
    if (!this->nonceFuckeryMigrated_)
    {
        this->nonceFuckeryEnabled = this->abnormalNonceDetection.getValue() ||
                                    this->normalNonceDetection.getValue();
        this->nonceFuckeryMigrated_ = true;
    }
}

Settings &Settings::instance()
{
    return *instance_;
}

Settings *getSettings()
{
    return &Settings::instance();
}

}  // namespace chatterino
