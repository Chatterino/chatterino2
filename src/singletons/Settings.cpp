#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/WindowManager.hpp"
#include "util/PersistSignalVector.hpp"
#include "util/WindowsHelper.hpp"

namespace chatterino {

ConcurrentSettings *concurrentInstance_{};

ConcurrentSettings::ConcurrentSettings()
    // NOTE: these do not get deleted
    : highlightedMessages(*new SignalVector<HighlightPhrase>())
    , highlightedUsers(*new SignalVector<HighlightPhrase>())
    , blacklistedUsers(*new SignalVector<HighlightBlacklistUser>())
    , ignoredMessages(*new SignalVector<IgnorePhrase>())
    , mutedChannels(*new SignalVector<QString>())
    , filterRecords(*new SignalVector<FilterRecordPtr>())
    , moderationActions(*new SignalVector<ModerationAction>)
{
    persist(this->highlightedMessages, "/highlighting/highlights");
    persist(this->blacklistedUsers, "/highlighting/blacklist");
    persist(this->highlightedUsers, "/highlighting/users");
    persist(this->ignoredMessages, "/ignore/phrases");
    persist(this->mutedChannels, "/pings/muted");
    persist(this->filterRecords, "/filtering/filters");
    // tagged users?
    persist(this->moderationActions, "/moderation/actions");
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

#ifdef USEWINSDK
    this->autorun = isRegisteredForStartup();
    this->autorun.connect(
        [](bool autorun) { setRegisteredForStartup(autorun); }, false);
#endif
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
