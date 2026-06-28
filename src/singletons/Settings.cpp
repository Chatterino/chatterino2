// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
#include "common/QLogging.hpp"
#include "controllers/filters/FilterRecord.hpp"
#include "controllers/highlights/HighlightBadge.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/highlights/types/All.hpp"  // IWYU pragma: keep
#include "controllers/highlights/types/YourMessagesHighlight.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "controllers/nicknames/Nickname.hpp"
#include "debug/Benchmark.hpp"
#include "pajlada/settings/signalargs.hpp"
#include "util/Backup.hpp"
#include "util/WindowsHelper.hpp"

#include <pajlada/signals/scoped-connection.hpp>

namespace {

using namespace chatterino;
using namespace Qt::Literals;

template <typename T>
void initializeSignalVector(pajlada::Signals::SignalHolder &signalHolder,
                            ChatterinoSetting<std::vector<T>> &setting,
                            SignalVector<T> &vec)
{
    // Fill the SignalVector up with initial values
    for (auto &&item : setting.getValue())
    {
        vec.append(item);
    }

    // Set up a signal to
    signalHolder.managedConnect(vec.delayedItemsChanged, [&] {
        setting.setValue(vec.raw());
    });
}

}  // namespace

namespace chatterino {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoSettings;

}  // namespace

class SettingsPrivate
{
public:
    ChatterinoSetting<std::vector<highlights::AllHighlights>>
        sharedHighlightsSetting = {
            "/highlighting/highlights2",
    };

    /// Old version 0 message highlights
    ChatterinoSetting<std::vector<HighlightPhrase>> highlightedMessagesSetting =
        {
            "/highlighting/highlights",
    };
    /// Old version 0 user highlights
    ChatterinoSetting<std::vector<HighlightPhrase>> highlightedUsersSetting = {
        "/highlighting/users",
    };
    /// Old version 0 badge highlights
    ChatterinoSetting<std::vector<HighlightBadge>> highlightedBadgesSetting = {
        "/highlighting/badges",
    };

    BoolSetting enableSelfHighlight = {
        "/highlighting/selfHighlight/nameIsHighlightKeyword",
        true,
    };
    BoolSetting showSelfHighlightInMentions = {
        "/highlighting/selfHighlight/showSelfHighlightInMentions",
        true,
    };
    BoolSetting enableSelfHighlightSound = {
        "/highlighting/selfHighlight/enableSound",
        true,
    };
    BoolSetting enableSelfHighlightTaskbar = {
        "/highlighting/selfHighlight/enableTaskbarFlashing",
        true,
    };
    QStringSetting selfHighlightSoundUrl = {
        "/highlighting/selfHighlightSoundUrl",
        "",
    };
    QStringSetting selfHighlightColor = {
        "/highlighting/selfHighlightColor",
        "",
    };

    BoolSetting enableSelfMessageHighlight = {
        "/highlighting/selfMessageHighlight/enabled",
        false,
    };
    BoolSetting showSelfMessageHighlightInMentions = {
        "/highlighting/selfMessageHighlight/showInMentions",
        false,
    };
    QStringSetting selfMessageHighlightColor = {
        "/highlighting/selfMessageHighlight/color",
        "",
    };

    BoolSetting enableWhisperHighlight = {
        "/highlighting/whisperHighlight/whispersHighlighted",
        true,
    };
    BoolSetting enableWhisperHighlightSound = {
        "/highlighting/whisperHighlight/enableSound",
        false,
    };
    BoolSetting enableWhisperHighlightTaskbar = {
        "/highlighting/whisperHighlight/enableTaskbarFlashing",
        false,
    };
    QStringSetting whisperHighlightSoundUrl = {
        "/highlighting/whisperHighlightSoundUrl",
        "",
    };
    QStringSetting whisperHighlightColor = {
        "/highlighting/whisperHighlightColor",
        "",
    };

    BoolSetting enableRedeemedHighlight = {
        "/highlighting/redeemedHighlight/highlighted",
        true,
    };
    QStringSetting redeemedHighlightColor = {
        "/highlighting/redeemedHighlightColor",
        "",
    };

    BoolSetting enableFirstMessageHighlight = {
        "/highlighting/firstMessageHighlight/highlighted",
        true,
    };
    QStringSetting firstMessageHighlightColor = {
        "/highlighting/firstMessageHighlightColor",
        "",
    };

    BoolSetting enableElevatedMessageHighlight = {
        "/highlighting/elevatedMessageHighlight/highlighted",
        true,
    };
    QStringSetting elevatedMessageHighlightColor = {
        "/highlighting/elevatedMessageHighlight/color",
        "",
    };

    BoolSetting enableSubHighlight = {
        "/highlighting/subHighlight/subsHighlighted",
        true,
    };
    BoolSetting enableSubHighlightSound = {
        "/highlighting/subHighlight/enableSound",
        false,
    };
    BoolSetting enableSubHighlightTaskbar = {
        "/highlighting/subHighlight/enableTaskbarFlashing",
        false,
    };
    QStringSetting subHighlightSoundUrl = {
        "/highlighting/subHighlightSoundUrl",
        "",
    };

    QStringSetting subHighlightColor = {
        "/highlighting/subHighlightColor",
        "",
    };

    BoolSetting enableThreadHighlight = {
        "/highlighting/thread/nameIsHighlightKeyword",
        true,
    };
    BoolSetting showThreadHighlightInMentions = {
        "/highlighting/thread/showSelfHighlightInMentions",
        true,
    };
    BoolSetting enableThreadHighlightSound = {
        "/highlighting/thread/enableSound",
        true,
    };
    BoolSetting enableThreadHighlightTaskbar = {
        "/highlighting/thread/enableTaskbarFlashing",
        true,
    };
    QStringSetting threadHighlightSoundUrl = {
        "/highlighting/threadHighlightSoundUrl",
        "",
    };
    QStringSetting threadHighlightColor = {
        "/highlighting/threadHighlightColor",
        "",
    };

    BoolSetting enableAutomodHighlight = {
        "/highlighting/automod/enabled",
        true,
    };
    BoolSetting showAutomodInMentions = {
        "/highlighting/automod/showInMentions",
        false,
    };
    BoolSetting enableAutomodHighlightSound = {
        "/highlighting/automod/enableSound",
        false,
    };
    BoolSetting enableAutomodHighlightTaskbar = {
        "/highlighting/automod/enableTaskbarFlashing",
        false,
    };
    QStringSetting automodHighlightSoundUrl = {
        "/highlighting/automod/soundUrl",
        "",
    };
    QStringSetting automodHighlightColor = {
        "/highlighting/automod/color",
        "",
    };

    BoolSetting enableWatchStreakHighlight = {
        "/highlighting/watchStreak/enabled",
        true,
    };
    QStringSetting watchStreakHighlightColor = {
        "/highlighting/watchStreak/color",
        "",
    };
};

std::vector<std::weak_ptr<pajlada::Settings::SettingData>> _settings;

void _actuallyRegisterSetting(
    std::weak_ptr<pajlada::Settings::SettingData> setting)
{
    _settings.push_back(std::move(setting));
}

void Settings::migrate(bool isTest)
{
    bool ranMigration = false;

    auto currentVersion = this->settingsVersion.getValue();

    if (currentVersion < 1)
    {
        qCInfo(LOG) << "Migrating highlights";
        this->migrateHighlights(isTest);
        currentVersion = 1;
        ranMigration = true;
    }

    this->settingsVersion.setValue(currentVersion);

    if (ranMigration)
    {
        // TODO: IS THIS LEGAL?
        qCInfo(LOG) << "Saving settings after migrations";
        this->requestSave();
    }
}

void Settings::migrateHighlights(bool isTest)
{
    using namespace chatterino::highlights;

    // TODO: This is not necessary in release - remove this
    this->p->sharedHighlightsSetting.setValue({});

    {
        YourUsernameHighlight h;

        if (const auto &s = this->p->enableSelfHighlight; s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        if (const auto &s = this->p->showSelfHighlightInMentions;
            s.hasValueBeenSet())
        {
            h.outcome.showInMentions = s.getValue();
        }

        if (const auto &s = this->p->enableSelfHighlightTaskbar;
            s.hasValueBeenSet())
        {
            h.outcome.alert = s.getValue();
        }

        if (const auto &s = this->p->enableSelfHighlightSound;
            s.hasValueBeenSet())
        {
            h.outcome.playSound = s.getValue();
        }

        if (const auto &s = this->p->selfHighlightSoundUrl; s.hasValueBeenSet())
        {
            h.outcome.customSoundURL = s.getValue();
        }

        if (const auto &s = this->p->selfHighlightColor; s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        WhispersHighlight h;

        if (const auto &s = this->p->enableWhisperHighlight;
            s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        // Whisper highlights do not support "show in mentions" - no setting to migrate

        if (const auto &s = this->p->enableWhisperHighlightTaskbar;
            s.hasValueBeenSet())
        {
            h.outcome.alert = s.getValue();
        }

        if (const auto &s = this->p->enableWhisperHighlightSound;
            s.hasValueBeenSet())
        {
            h.outcome.playSound = s.getValue();
        }

        if (const auto &s = this->p->whisperHighlightSoundUrl;
            s.hasValueBeenSet())
        {
            h.outcome.customSoundURL = s.getValue();
        }

        if (const auto &s = this->p->whisperHighlightColor; s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        SubscriptionsHighlight h;

        if (const auto &s = this->p->enableSubHighlight; s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        // Sub highlights do not support "show in mentions" - no setting to migrate

        if (const auto &s = this->p->enableSubHighlightTaskbar;
            s.hasValueBeenSet())
        {
            h.outcome.alert = s.getValue();
        }

        if (const auto &s = this->p->enableSubHighlightSound;
            s.hasValueBeenSet())
        {
            h.outcome.playSound = s.getValue();
        }

        if (const auto &s = this->p->subHighlightSoundUrl; s.hasValueBeenSet())
        {
            h.outcome.customSoundURL = s.getValue();
        }

        if (const auto &s = this->p->subHighlightColor; s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        ChannelPointsHighlight h;

        if (const auto &s = this->p->enableRedeemedHighlight;
            s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        // This does not support "show in mentions" - no setting to migrate
        // This does not support "flash taskbar" - no setting to migrate
        // This does not support "play sound" - no setting to migrate

        if (const auto &s = this->p->redeemedHighlightColor;
            s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        FirstMessageHighlight h;

        if (const auto &s = this->p->enableFirstMessageHighlight;
            s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        // This does not support "show in mentions" - no setting to migrate
        // This does not support "flash taskbar" - no setting to migrate
        // This does not support "play sound" - no setting to migrate

        if (const auto &s = this->p->firstMessageHighlightColor;
            s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        HypeChatHighlight h;

        if (const auto &s = this->p->enableElevatedMessageHighlight;
            s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        // This does not support "show in mentions" - no setting to migrate
        // This does not support "flash taskbar" - no setting to migrate
        // This does not support "play sound" - no setting to migrate

        if (const auto &s = this->p->elevatedMessageHighlightColor;
            s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        // TODO: This order is probably ALSO wrong.
        SubscribedThreadHighlight h;

        if (const auto &s = this->p->enableThreadHighlight; s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        if (const auto &s = this->p->showThreadHighlightInMentions;
            s.hasValueBeenSet())
        {
            h.outcome.showInMentions = s.getValue();
        }

        if (const auto &s = this->p->enableThreadHighlightTaskbar;
            s.hasValueBeenSet())
        {
            h.outcome.alert = s.getValue();
        }

        if (const auto &s = this->p->enableThreadHighlightSound;
            s.hasValueBeenSet())
        {
            h.outcome.playSound = s.getValue();
        }

        if (const auto &s = this->p->threadHighlightSoundUrl;
            s.hasValueBeenSet())
        {
            h.outcome.customSoundURL = s.getValue();
        }

        if (const auto &s = this->p->threadHighlightColor; s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        // TODO: This should _actually_ be migrated _AFTER_ message highlights.
        AutomodCaughtHighlight h;

        if (const auto &s = this->p->enableAutomodHighlight;
            s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        if (const auto &s = this->p->showAutomodInMentions; s.hasValueBeenSet())
        {
            h.outcome.showInMentions = s.getValue();
        }

        if (const auto &s = this->p->enableAutomodHighlightTaskbar;
            s.hasValueBeenSet())
        {
            h.outcome.alert = s.getValue();
        }

        if (const auto &s = this->p->enableAutomodHighlightSound;
            s.hasValueBeenSet())
        {
            h.outcome.playSound = s.getValue();
        }

        if (const auto &s = this->p->automodHighlightSoundUrl;
            s.hasValueBeenSet())
        {
            h.outcome.customSoundURL = s.getValue();
        }

        if (const auto &s = this->p->automodHighlightColor; s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        WatchStreakHighlight h;

        if (const auto &s = this->p->enableWatchStreakHighlight;
            s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        // This does not support "show in mentions" - no setting to migrate
        // This does not support "flash taskbar" - no setting to migrate
        // This does not support "play sound" - no setting to migrate

        if (const auto &s = this->p->watchStreakHighlightColor;
            s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    {
        YourMessagesHighlight h;

        if (const auto &s = this->p->enableSelfMessageHighlight;
            s.hasValueBeenSet())
        {
            h.enabled = s.getValue();
        }

        if (const auto &s = this->p->showSelfMessageHighlightInMentions;
            s.hasValueBeenSet())
        {
            h.outcome.showInMentions = s.getValue();
        }

        // This does not support "flash taskbar" - no setting to migrate
        // This does not support "play sound" - no setting to migrate

        if (const auto &s = this->p->selfMessageHighlightColor;
            s.hasValueBeenSet())
        {
            h.outcome.setBackgroundColor(s.getValue());
        }

        this->p->sharedHighlightsSetting.push_back(h);
    }

    // this migration ID is used for tests to provide a stable "uuid" replacement for created user defined highlights,
    // and also to provide some output to the user in their logs for how many highlights were migrated
    int migrationID = 0;

    for (const auto &from : this->p->highlightedMessagesSetting.getValue())
    {
        auto generatedId = [&] {
            migrationID += 1;

            if (isTest)
            {
                return QString("test-%1").arg(migrationID);
            }

            auto v4Uuid = QUuid::createUuid();
            return v4Uuid.toString(QUuid::StringFormat::WithoutBraces);
        }();

        MessageHighlight to{generatedId};

        to.setPattern(from.pattern);
        to.outcome.showInMentions = from.showInMentions;
        to.outcome.alert = from.hasAlert;
        to.setRegex(from.isRegex);
        to.setCaseSensitive(from.isCaseSensitive);
        to.outcome.playSound = from.hasSound;
        if (!from.soundUrl.isEmpty())
        {
            to.outcome.customSoundURL = from.soundUrl;
        }
        if (auto fromColor = from.color; fromColor.isValid())
        {
            to.outcome.setBackgroundColor(fromColor);
        }

        this->p->sharedHighlightsSetting.push_back(to);
    }

    for (const auto &from : this->p->highlightedUsersSetting.getValue())
    {
        auto generatedId = [&] {
            migrationID += 1;

            if (isTest)
            {
                return QString("test-user-%1").arg(migrationID);
            }

            auto v4Uuid = QUuid::createUuid();
            return v4Uuid.toString(QUuid::StringFormat::WithoutBraces);
        }();

        UserHighlight to{generatedId};

        to.setUsername(from.pattern);
        to.outcome.showInMentions = from.showInMentions;
        to.outcome.alert = from.hasAlert;
        to.outcome.playSound = from.hasSound;
        if (!from.soundUrl.isEmpty())
        {
            to.outcome.customSoundURL = from.soundUrl;
        }
        if (auto fromColor = from.color; fromColor.isValid())
        {
            to.outcome.setBackgroundColor(fromColor);
        }

        this->p->sharedHighlightsSetting.push_back(to);
    }

    for (const auto &from : this->p->highlightedBadgesSetting.getValue())
    {
        auto generatedId = [&] {
            migrationID += 1;

            if (isTest)
            {
                return QString("test-badge-%1").arg(migrationID);
            }

            auto v4Uuid = QUuid::createUuid();
            return v4Uuid.toString(QUuid::StringFormat::WithoutBraces);
        }();

        BadgeHighlight to{generatedId};

        to.setBadgeName(from.badgeName);
        to.setDisplayName(from.displayName);
        to.outcome.showInMentions = from.showInMentions;
        to.outcome.alert = from.hasAlert;
        to.outcome.playSound = from.hasSound;
        to.outcome.customSoundURL = from.soundUrl;
        if (auto fromColor = from.color; fromColor.isValid())
        {
            to.outcome.setBackgroundColor(fromColor);
        }

        this->p->sharedHighlightsSetting.push_back(to);
    }

    this->p->sharedHighlightsSetting.push_back(
        UncategorizedNotificationHighlight{});
}

bool Settings::isBlacklistedUser(const QString &username)
{
    auto items = this->blacklistedUsers.readOnly();

    for (const auto &blacklistedUser : *items)
    {
        if (blacklistedUser.isMatch(username))
        {
            return true;
        }
    }

    return false;
}

bool Settings::isMutedChannel(const QString &channelName)
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

std::optional<QString> Settings::matchNickname(const QString &usernameText)
{
    auto nicknames = this->nicknames.readOnly();

    for (const auto &nickname : *nicknames)
    {
        if (auto nicknameText = nickname.match(usernameText))
        {
            return nicknameText;
        }
    }

    return std::nullopt;
}

void Settings::mute(const QString &channelName)
{
    if (!this->isMutedChannel(channelName))
    {
        this->mutedChannels.append(channelName);
    }
}

void Settings::unmute(const QString &channelName)
{
    for (std::vector<int>::size_type i = 0;
         i != this->mutedChannels.raw().size(); i++)
    {
        if (this->mutedChannels.raw()[i].toLower() == channelName.toLower())
        {
            this->mutedChannels.removeAt(i);
            i--;
        }
    }
}

bool Settings::toggleMutedChannel(const QString &channelName)
{
    if (this->isMutedChannel(channelName))
    {
        this->unmute(channelName);
        return false;
    }
    else
    {
        this->mutedChannels.append(channelName);
        return true;
    }
}

Settings *Settings::instance_ = nullptr;

Settings::Settings(const Args &args, const QString &settingsDirectory,
                   const SettingsArgs &settingsArgs)
    : prevInstance_(Settings::instance_)
    , disableSaving(args.dontSaveSettings)
    , p(std::make_unique<SettingsPrivate>())
{
    QString settingsPath = settingsDirectory + "/settings.json";

    // get global instance of the settings library
    auto settingsInstance = pajlada::Settings::SettingManager::getInstance();

    if (settingsArgs.isTest)
    {
        qCInfo(LOG) << "Loading settings from" << settingsPath;
        settingsInstance->load(qPrintable(settingsPath));
    }
    else
    {
        backup::loadWithBackups(
            backup::FileData{
                .fileName = u"settings.json"_s,
                .directory = settingsDirectory,
                .fileKind = u"Settings"_s,
                .fileDescription =
                    u"This file contains the main application settings such as accounts and hotkeys."_s,
            },
            [&]() -> ExpectedStr<void> {
                using LoadError = pajlada::Settings::SettingManager::LoadError;
                auto err = settingsInstance->load(qPrintable(settingsPath));
                switch (err)
                {
                    case LoadError::NoError:
                        return {};  // ok
                    case LoadError::CannotOpenFile:
                        return makeUnexpected(u"Failed to open '" %
                                              settingsPath % '\'');
                    case LoadError::FileHandleError:
                        return makeUnexpected("File handle error");
                    case LoadError::FileReadError:
                        return makeUnexpected("Failed to read file");
                    case LoadError::FileSeekError:
                        return makeUnexpected("Failed to seek in file");
                    case LoadError::JSONParseError:
                        return makeUnexpected("File contained malformed JSON");
                }
                assert(false);
                return makeUnexpected("Unknown error");
            });
    }

    settingsInstance->setBackupEnabled(true);
    settingsInstance->setBackupSlots(9);
    settingsInstance->saveMethod = static_cast<
        pajlada::Settings::SettingManager::SaveMethod>(
        static_cast<uint64_t>(
            pajlada::Settings::SettingManager::SaveMethod::SaveManually) |
        static_cast<uint64_t>(
            pajlada::Settings::SettingManager::SaveMethod::OnlySaveIfChanged));

    // Run setting migrations
    if (settingsArgs.runMigrations)
    {
        this->migrate(settingsArgs.isTest);
    }

    initializeSignalVector(this->signalHolder, this->p->sharedHighlightsSetting,
                           this->sharedHighlights);
    initializeSignalVector(this->signalHolder, this->blacklistedUsersSetting,
                           this->blacklistedUsers);
    initializeSignalVector(this->signalHolder, this->ignoredMessagesSetting,
                           this->ignoredMessages);
    initializeSignalVector(this->signalHolder, this->mutedChannelsSetting,
                           this->mutedChannels);
    initializeSignalVector(this->signalHolder, this->filterRecordsSetting,
                           this->filterRecords);
    initializeSignalVector(this->signalHolder, this->nicknamesSetting,
                           this->nicknames);
    initializeSignalVector(this->signalHolder, this->moderationActionsSetting,
                           this->moderationActions);
    initializeSignalVector(this->signalHolder, this->loggedChannelsSetting,
                           this->loggedChannels);

    instance_ = this;

#ifdef USEWINSDK
    this->autorun = isRegisteredForStartup();
    this->autorun.connect(
        [](bool autorun) {
            setRegisteredForStartup(autorun);
        },
        false);
#endif
}

Settings::~Settings()
{
    Settings::instance_ = this->prevInstance_;
}

pajlada::Settings::SettingManager::SaveResult Settings::requestSave() const
{
    if (this->disableSaving)
    {
        return pajlada::Settings::SettingManager::SaveResult::Skipped;
    }

    return pajlada::Settings::SettingManager::gSave();
}

void Settings::saveSnapshot()
{
    BenchmarkGuard benchmark("Settings::saveSnapshot");

    rapidjson::Document *d = new rapidjson::Document(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType &a = d->GetAllocator();

    for (const auto &weakSetting : _settings)
    {
        auto setting = weakSetting.lock();
        if (!setting)
        {
            continue;
        }

        rapidjson::Value key(setting->getPath().c_str(), a);
        auto *curVal = setting->unmarshalJSON();
        if (curVal == nullptr)
        {
            continue;
        }

        rapidjson::Value val;
        val.CopyFrom(*curVal, a);
        d->AddMember(key.Move(), val.Move(), a);
    }

    // log("Snapshot state: {}", rj::stringify(*d));

    this->snapshot_.reset(d);
}

void Settings::restoreSnapshot()
{
    if (!this->snapshot_)
    {
        return;
    }

    BenchmarkGuard benchmark("Settings::restoreSnapshot");

    const auto &snapshot = *(this->snapshot_.get());

    if (!snapshot.IsObject())
    {
        return;
    }

    for (const auto &weakSetting : _settings)
    {
        auto setting = weakSetting.lock();
        if (!setting)
        {
            continue;
        }

        const char *path = setting->getPath().c_str();

        if (!snapshot.HasMember(path))
        {
            continue;
        }

        pajlada::Settings::SignalArgs args;
        args.compareBeforeSet = true;

        setting->marshalJSON(snapshot[path], std::move(args));
    }
}

void Settings::disableSave()
{
    this->disableSaving = true;
}

bool Settings::shouldSendHelixChat() const
{
    switch (this->chatSendProtocol.getEnum())
    {
        case ChatSendProtocol::Helix:
            return true;
        case ChatSendProtocol::Default:
        case ChatSendProtocol::IRC:
            return false;
        default:
            assert(false && "Invalid chat protocol value");
            return false;
    }
}

float Settings::getClampedUiScale() const
{
    return std::clamp(this->uiScale.getValue(), 0.2F, 10.F);
}

void Settings::setClampedUiScale(float value)
{
    this->uiScale.setValue(std::clamp(value, 0.2F, 10.F));
}

float Settings::getClampedOverlayScale() const
{
    return std::clamp(this->overlayScaleFactor.getValue(), 0.2F, 10.F);
}

void Settings::setClampedOverlayScale(float value)
{
    this->overlayScaleFactor.setValue(std::clamp(value, 0.2F, 10.F));
}

Settings &Settings::instance()
{
    assert(instance_ != nullptr);

    return *instance_;
}

Settings *getSettings()
{
    return &Settings::instance();
}

}  // namespace chatterino
