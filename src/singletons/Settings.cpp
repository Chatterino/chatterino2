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
    : highlightedMessages(*new SignalVector<HighlightPhrase>())
    , highlightedUsers(*new SignalVector<HighlightPhrase>())
    , blacklistedUsers(*new SignalVector<HighlightBlacklistUser>())
    , ignoredMessages(*new SignalVector<IgnorePhrase>())
{
}

bool ConcurrentSettings::isHighlightedUser(const QString &username)
{
    for (const auto &highlightedUser : this->highlightedUsers)
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

    persist(this->highlightedMessages, "/highlighting/highlights");
    persist(this->blacklistedUsers, "/highlighting/blacklist");
    persist(this->highlightedUsers, "/highlighting/users");
    persist(this->ignoredMessages, "/ignore/phrases");

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
