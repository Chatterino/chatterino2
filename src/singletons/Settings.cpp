// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/Settings.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
#include "controllers/filters/FilterRecord.hpp"
#include "controllers/highlights/HighlightBadge.hpp"
#include "controllers/highlights/HighlightBlacklistUser.hpp"
#include "controllers/highlights/HighlightPhrase.hpp"
#include "controllers/highlights/SharedHighlight.hpp"
#include "controllers/ignores/IgnorePhrase.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "controllers/nicknames/Nickname.hpp"
#include "debug/Benchmark.hpp"
#include "pajlada/settings/signalargs.hpp"
#include "util/WindowsHelper.hpp"

#include <pajlada/signals/scoped-connection.hpp>

namespace {

using namespace chatterino;

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

std::vector<std::weak_ptr<pajlada::Settings::SettingData>> _settings;

void _actuallyRegisterSetting(
    std::weak_ptr<pajlada::Settings::SettingData> setting)
{
    _settings.push_back(std::move(setting));
}

void Settings::migrate()
{
    auto currentVersion = this->version.getValue();

    if (currentVersion < 1)
    {
        this->migrateHighlights();
        // currentVersion = 1;
    }

    this->version.setValue(currentVersion);
}

void Settings::migrateHighlights()
{
    qInfo() << "XXX: MIGRATE HIGHLIGHTS";
    // TODO: Migrate
    //  - /highlighting/subHighlight/subsHighlighted
    //  - /highlighting/subHighlight/enableSound
    //  - /highlighting/subHighlight/enableTaskbarFlashing
    //  to /definedHighlights/subHighlight

    auto n = this->sharedHighlightsSetting.getValue();

    // TODO: Ensure the order is correct when a user first migrates

    {
        const QString uuid = "subhighlight";
        auto enabled = this->enableSubHighlight.getValue();
        auto soundEnabled = this->enableSubHighlightSound.getValue();
        auto taskbarEnabled = this->enableSubHighlightTaskbar.getValue();
        auto soundUrl = this->subHighlightSoundUrl.getValue();
        auto color = this->subHighlightColor.getValue();

        auto newPhrase = QMap<QString, QJsonValue>{
            {"enabled", enabled},
            {"showInMentions", false},
            {"flashTaskbar", taskbarEnabled},
            {"enableRegex", false},
            {"caseSensitive", false},
            {"playSound", soundEnabled},
            {"customSound", soundUrl},
            {"color", color},
        };

        bool found = false;

        for (auto &highlight : n)
        {
            if (highlight.id == uuid)
            {
                highlight.playSound = soundEnabled;
                highlight.alert = taskbarEnabled;
                highlight.customSoundURL = soundUrl;
                *highlight.backgroundColor = QColor::fromString(color);
                found = true;
                break;
            }
        }

        if (!found)
        {
            n.push_back(SharedHighlight{
                .id = uuid,
                .name = "xd",
                .enabled = true,
                .pattern = "asd",
                .showInMentions = false,
                .alert = taskbarEnabled,
                .playSound = soundEnabled,
                .customSoundURL = soundUrl,
                .backgroundColor =
                    std::make_shared<QColor>(QColor::fromString(color)),
                .isRegex = false,
                .isCaseSensitive = false,
            });
        }
    }

    this->sharedHighlightsSetting.setValue(n);

    qInfo() << "XXX:" << n;
}

bool Settings::isHighlightedUser(const QString &username)
{
    auto items = this->highlightedUsers.readOnly();

    for (const auto &highlightedUser : *items)
    {
        if (highlightedUser.isMatch(username))
        {
            return true;
        }
    }

    return false;
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

Settings::Settings(const Args &args, const QString &settingsDirectory)
    : prevInstance_(Settings::instance_)
    , disableSaving(args.dontSaveSettings)
{
    QString settingsPath = settingsDirectory + "/settings.json";

    // get global instance of the settings library
    auto settingsInstance = pajlada::Settings::SettingManager::getInstance();

    settingsInstance->load(qPrintable(settingsPath));

    settingsInstance->setBackupEnabled(true);
    settingsInstance->setBackupSlots(9);
    settingsInstance->saveMethod = static_cast<
        pajlada::Settings::SettingManager::SaveMethod>(
        static_cast<uint64_t>(
            pajlada::Settings::SettingManager::SaveMethod::SaveManually) |
        static_cast<uint64_t>(
            pajlada::Settings::SettingManager::SaveMethod::OnlySaveIfChanged));

    // Run setting migrations
    this->migrate();

    initializeSignalVector(this->signalHolder, this->sharedHighlightsSetting,
                           this->sharedHighlights);
    initializeSignalVector(this->signalHolder, this->highlightedMessagesSetting,
                           this->highlightedMessages);
    initializeSignalVector(this->signalHolder, this->highlightedUsersSetting,
                           this->highlightedUsers);
    initializeSignalVector(this->signalHolder, this->highlightedBadgesSetting,
                           this->highlightedBadges);
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
