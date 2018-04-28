#include "singletons/settingsmanager.hpp"

#include "application.hpp"
#include "debug/log.hpp"
#include "singletons/pathmanager.hpp"
#include "singletons/resourcemanager.hpp"
#include "singletons/windowmanager.hpp"

using namespace chatterino::messages;

namespace chatterino {
namespace singletons {

std::vector<std::weak_ptr<pajlada::Settings::ISettingData>> _settings;

void _actuallyRegisterSetting(std::weak_ptr<pajlada::Settings::ISettingData> setting)
{
    _settings.push_back(setting);
}

SettingManager::SettingManager()
    : _ignoredKeywords(new std::vector<QString>)
{
    qDebug() << "init SettingManager";

    this->wordFlagsListener.addSetting(this->showTimestamps);
    this->wordFlagsListener.addSetting(this->showBadges);
    this->wordFlagsListener.addSetting(this->enableBttvEmotes);
    this->wordFlagsListener.addSetting(this->enableEmojis);
    this->wordFlagsListener.addSetting(this->enableFfzEmotes);
    this->wordFlagsListener.addSetting(this->enableTwitchEmotes);
    this->wordFlagsListener.cb = [this](auto) {
        this->updateWordTypeMask();  //
    };
}

void SettingManager::initialize()
{
    this->moderationActions.connect([this](auto, auto) { this->updateModerationActions(); });
    this->ignoredKeywords.connect([this](auto, auto) { this->updateIgnoredKeywords(); });

    this->timestampFormat.connect([](auto, auto) {
        auto app = getApp();
        app->windows->layoutVisibleChatWidgets();
    });
}

MessageElement::Flags SettingManager::getWordFlags()
{
    return this->wordFlags;
}

bool SettingManager::isIgnoredEmote(const QString &)
{
    return false;
}

void SettingManager::load()
{
    auto app = getApp();
    QString settingsPath = app->paths->settingsFolderPath + "/settings.json";

    pajlada::Settings::SettingManager::load(qPrintable(settingsPath));
}

void SettingManager::updateWordTypeMask()
{
    uint32_t newMaskUint = MessageElement::Text;

    if (this->showTimestamps) {
        newMaskUint |= MessageElement::Timestamp;
    }

    newMaskUint |=
        enableTwitchEmotes ? MessageElement::TwitchEmoteImage : MessageElement::TwitchEmoteText;
    newMaskUint |= enableFfzEmotes ? MessageElement::FfzEmoteImage : MessageElement::FfzEmoteText;
    newMaskUint |=
        enableBttvEmotes ? MessageElement::BttvEmoteImage : MessageElement::BttvEmoteText;
    newMaskUint |= enableEmojis ? MessageElement::EmojiImage : MessageElement::EmojiText;

    newMaskUint |= MessageElement::BitsAmount;
    newMaskUint |= enableGifAnimations ? MessageElement::BitsAnimated : MessageElement::BitsStatic;

    if (this->showBadges) {
        newMaskUint |= MessageElement::Badges;
    }

    newMaskUint |= MessageElement::Username;

    newMaskUint |= MessageElement::AlwaysShow;
    newMaskUint |= MessageElement::Collapsed;

    MessageElement::Flags newMask = static_cast<MessageElement::Flags>(newMaskUint);

    if (newMask != this->wordFlags) {
        this->wordFlags = newMask;

        this->wordFlagsChanged.invoke();
    }
}

void SettingManager::saveSnapshot()
{
    rapidjson::Document *d = new rapidjson::Document(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType &a = d->GetAllocator();

    for (const auto &weakSetting : _settings) {
        auto setting = weakSetting.lock();
        if (!setting) {
            continue;
        }

        rapidjson::Value key(setting->getPath().c_str(), a);
        rapidjson::Value val = setting->marshalInto(*d);
        d->AddMember(key.Move(), val.Move(), a);
    }

    this->snapshot.reset(d);

    debug::Log("hehe: {}", pajlada::Settings::SettingManager::stringify(*d));
}

void SettingManager::recallSnapshot()
{
    if (!this->snapshot) {
        return;
    }

    const auto &snapshotObject = this->snapshot->GetObject();

    for (const auto &weakSetting : _settings) {
        auto setting = weakSetting.lock();
        if (!setting) {
            debug::Log("Error stage 1 of loading");
            continue;
        }

        const char *path = setting->getPath().c_str();

        if (!snapshotObject.HasMember(path)) {
            debug::Log("Error stage 2 of loading");
            continue;
        }

        setting->unmarshalValue(snapshotObject[path]);
    }
}

std::vector<ModerationAction> SettingManager::getModerationActions() const
{
    return this->_moderationActions;
}

const std::shared_ptr<std::vector<QString>> SettingManager::getIgnoredKeywords() const
{
    return this->_ignoredKeywords;
}

void SettingManager::updateModerationActions()
{
    auto app = getApp();

    this->_moderationActions.clear();

    static QRegularExpression newLineRegex("(\r\n?|\n)+");
    static QRegularExpression replaceRegex("[!/.]");
    static QRegularExpression timeoutRegex("^[./]timeout.* (\\d+)");
    QStringList list = this->moderationActions.getValue().split(newLineRegex);

    int multipleTimeouts = 0;

    for (QString &str : list) {
        if (timeoutRegex.match(str).hasMatch()) {
            multipleTimeouts++;
            if (multipleTimeouts > 1) {
                break;
            }
        }
    }

    for (int i = 0; i < list.size(); i++) {
        QString &str = list[i];

        if (str.isEmpty()) {
            continue;
        }

        auto timeoutMatch = timeoutRegex.match(str);

        if (timeoutMatch.hasMatch()) {
            if (multipleTimeouts > 1) {
                QString line1;
                QString line2;

                int amount = timeoutMatch.captured(1).toInt();

                if (amount < 60) {
                    line1 = QString::number(amount);
                    line2 = "s";
                } else if (amount < 60 * 60) {
                    line1 = QString::number(amount / 60);
                    line2 = "m";
                } else if (amount < 60 * 60 * 24) {
                    line1 = QString::number(amount / 60 / 60);
                    line2 = "h";
                } else {
                    line1 = QString::number(amount / 60 / 60 / 24);
                    line2 = "d";
                }

                this->_moderationActions.emplace_back(line1, line2, str);
            } else {
                this->_moderationActions.emplace_back(app->resources->buttonTimeout, str);
            }
        } else if (str.startsWith("/ban ")) {
            this->_moderationActions.emplace_back(app->resources->buttonBan, str);
        } else {
            QString xD = str;

            xD.replace(replaceRegex, "");

            this->_moderationActions.emplace_back(xD.mid(0, 2), xD.mid(2, 2), str);
        }
    }
}

void SettingManager::updateIgnoredKeywords()
{
    static QRegularExpression newLineRegex("(\r\n?|\n)+");

    auto items = new std::vector<QString>();

    for (const QString &line : this->ignoredKeywords.getValue().split(newLineRegex)) {
        QString line2 = line.trimmed();

        if (!line2.isEmpty()) {
            items->push_back(line2);
        }
    }

    this->_ignoredKeywords = std::shared_ptr<std::vector<QString>>(items);
}
}  // namespace singletons
}  // namespace chatterino
