#include "Resources.hpp"
#include "common/UrlFetch.hpp"

#include <QIcon>
#include <QPixmap>

namespace chatterino {

namespace {

inline Image *lli(const char *pixmapPath, qreal scale = 1)
{
    return new Image(new QPixmap(pixmapPath), scale);
}

template <typename Type>
inline bool ReadValue(const rapidjson::Value &object, const char *key, Type &out)
{
    if (!object.HasMember(key)) {
        return false;
    }

    const auto &value = object[key];

    if (!value.Is<Type>()) {
        return false;
    }

    out = value.Get<Type>();

    return true;
}

template <>
inline bool ReadValue<QString>(const rapidjson::Value &object, const char *key, QString &out)
{
    if (!object.HasMember(key)) {
        return false;
    }

    const auto &value = object[key];

    if (!value.IsString()) {
        return false;
    }

    out = value.GetString();

    return true;
}

template <>
inline bool ReadValue<std::vector<QString>>(const rapidjson::Value &object, const char *key,
                                            std::vector<QString> &out)
{
    if (!object.HasMember(key)) {
        return false;
    }

    const auto &value = object[key];

    if (!value.IsArray()) {
        return false;
    }

    for (const rapidjson::Value &innerValue : value.GetArray()) {
        if (!innerValue.IsString()) {
            return false;
        }

        out.emplace_back(innerValue.GetString());
    }

    return true;
}

// Parse a single cheermote set (or "action") from the twitch api
inline bool ParseSingleCheermoteSet(Resources::JSONCheermoteSet &set,
                                    const rapidjson::Value &action)
{
    if (!action.IsObject()) {
        return false;
    }

    if (!ReadValue(action, "prefix", set.prefix)) {
        return false;
    }

    if (!ReadValue(action, "scales", set.scales)) {
        return false;
    }

    if (!ReadValue(action, "backgrounds", set.backgrounds)) {
        return false;
    }

    if (!ReadValue(action, "states", set.states)) {
        return false;
    }

    if (!ReadValue(action, "type", set.type)) {
        return false;
    }

    if (!ReadValue(action, "updated_at", set.updatedAt)) {
        return false;
    }

    if (!ReadValue(action, "priority", set.priority)) {
        return false;
    }

    // Tiers
    if (!action.HasMember("tiers")) {
        return false;
    }

    const auto &tiersValue = action["tiers"];

    if (!tiersValue.IsArray()) {
        return false;
    }

    for (const rapidjson::Value &tierValue : tiersValue.GetArray()) {
        Resources::JSONCheermoteSet::CheermoteTier tier;

        if (!tierValue.IsObject()) {
            return false;
        }

        if (!ReadValue(tierValue, "min_bits", tier.minBits)) {
            return false;
        }

        if (!ReadValue(tierValue, "id", tier.id)) {
            return false;
        }

        if (!ReadValue(tierValue, "color", tier.color)) {
            return false;
        }

        // Images
        if (!tierValue.HasMember("images")) {
            return false;
        }

        const auto &imagesValue = tierValue["images"];

        if (!imagesValue.IsObject()) {
            return false;
        }

        // Read images object
        for (const auto &imageBackgroundValue : imagesValue.GetObject()) {
            QString background = imageBackgroundValue.name.GetString();
            bool backgroundExists = false;
            for (const auto &bg : set.backgrounds) {
                if (background == bg) {
                    backgroundExists = true;
                    break;
                }
            }

            if (!backgroundExists) {
                continue;
            }

            const rapidjson::Value &imageBackgroundStates = imageBackgroundValue.value;
            if (!imageBackgroundStates.IsObject()) {
                continue;
            }

            // Read each key which represents a background
            for (const auto &imageBackgroundState : imageBackgroundStates.GetObject()) {
                QString state = imageBackgroundState.name.GetString();
                bool stateExists = false;
                for (const auto &_state : set.states) {
                    if (state == _state) {
                        stateExists = true;
                        break;
                    }
                }

                if (!stateExists) {
                    continue;
                }

                const rapidjson::Value &imageScalesValue = imageBackgroundState.value;
                if (!imageScalesValue.IsObject()) {
                    continue;
                }

                // Read each key which represents a scale
                for (const auto &imageScaleValue : imageScalesValue.GetObject()) {
                    QString scale = imageScaleValue.name.GetString();
                    bool scaleExists = false;
                    for (const auto &_scale : set.scales) {
                        if (scale == _scale) {
                            scaleExists = true;
                            break;
                        }
                    }

                    if (!scaleExists) {
                        continue;
                    }

                    const rapidjson::Value &imageScaleURLValue = imageScaleValue.value;
                    if (!imageScaleURLValue.IsString()) {
                        continue;
                    }

                    QString url = imageScaleURLValue.GetString();

                    bool ok = false;
                    qreal scaleNumber = scale.toFloat(&ok);
                    if (!ok) {
                        continue;
                    }

                    qreal chatterinoScale = 1 / scaleNumber;

                    auto image = new Image(url, chatterinoScale);

                    // TODO(pajlada): Fill in name and tooltip
                    tier.images[background][state][scale] = image;
                }
            }
        }

        set.tiers.emplace_back(tier);
    }

    return true;
}

// Look through the results of https://api.twitch.tv/kraken/bits/actions?channel_id=11148817 for
// cheermote sets or "Actions" as they are called in the API
inline void ParseCheermoteSets(std::vector<Resources::JSONCheermoteSet> &sets,
                               const rapidjson::Document &d)
{
    if (!d.IsObject()) {
        return;
    }

    if (!d.HasMember("actions")) {
        return;
    }

    const auto &actionsValue = d["actions"];

    if (!actionsValue.IsArray()) {
        return;
    }

    for (const auto &action : actionsValue.GetArray()) {
        Resources::JSONCheermoteSet set;
        bool res = ParseSingleCheermoteSet(set, action);

        if (res) {
            sets.emplace_back(set);
        }
    }
}

}  // namespace
Resources::Resources()
    : badgeStaff(lli(":/images/staff_bg.png"))
    , badgeAdmin(lli(":/images/admin_bg.png"))
    , badgeGlobalModerator(lli(":/images/globalmod_bg.png"))
    , badgeModerator(lli(":/images/moderator_bg.png"))
    , badgeTurbo(lli(":/images/turbo_bg.png"))
    , badgeBroadcaster(lli(":/images/broadcaster_bg.png"))
    , badgePremium(lli(":/images/twitchprime_bg.png"))
    , badgeVerified(lli(":/images/verified.png", 0.25))
    , badgeSubscriber(lli(":/images/subscriber.png", 0.25))
    , badgeCollapsed(lli(":/images/collapse.png"))
    , cheerBadge100000(lli(":/images/cheer100000"))
    , cheerBadge10000(lli(":/images/cheer10000"))
    , cheerBadge5000(lli(":/images/cheer5000"))
    , cheerBadge1000(lli(":/images/cheer1000"))
    , cheerBadge100(lli(":/images/cheer100"))
    , cheerBadge1(lli(":/images/cheer1"))
    , moderationmode_enabled(lli(":/images/moderatormode_enabled"))
    , moderationmode_disabled(lli(":/images/moderatormode_disabled"))
    , splitHeaderContext(lli(":/images/tool_moreCollapser_off16.png"))
    , buttonBan(lli(":/images/button_ban.png", 0.25))
    , buttonTimeout(lli(":/images/button_timeout.png", 0.25))
{
    this->split.left = QIcon(":/images/split/splitleft.png");
    this->split.right = QIcon(":/images/split/splitright.png");
    this->split.up = QIcon(":/images/split/splitup.png");
    this->split.down = QIcon(":/images/split/splitdown.png");
    this->split.move = QIcon(":/images/split/splitmove.png");

    this->buttons.ban = QPixmap(":/images/buttons/ban.png");
    this->buttons.unban = QPixmap(":/images/buttons/unban.png");
    this->buttons.mod = QPixmap(":/images/buttons/mod.png");
    this->buttons.unmod = QPixmap(":/images/buttons/unmod.png");

    qDebug() << "init ResourceManager";
}

void Resources::initialize()
{
    this->loadDynamicTwitchBadges();

    this->loadChatterinoBadges();
}

Resources::BadgeVersion::BadgeVersion(QJsonObject &&root)
    : badgeImage1x(new Image(root.value("image_url_1x").toString()))
    , badgeImage2x(new Image(root.value("image_url_2x").toString()))
    , badgeImage4x(new Image(root.value("image_url_4x").toString()))
    , description(root.value("description").toString().toStdString())
    , title(root.value("title").toString().toStdString())
    , clickAction(root.value("clickAction").toString().toStdString())
    , clickURL(root.value("clickURL").toString().toStdString())
{
}

void Resources::loadChannelData(const QString &roomID, bool bypassCache)
{
    QString url = "https://badges.twitch.tv/v1/badges/channels/" + roomID + "/display?language=en";

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.getJSON([this, roomID](QJsonObject &root) {
        QJsonObject sets = root.value("badge_sets").toObject();

        Resources::Channel &ch = this->channels[roomID];

        for (QJsonObject::iterator it = sets.begin(); it != sets.end(); ++it) {
            QJsonObject versions = it.value().toObject().value("versions").toObject();

            auto &badgeSet = ch.badgeSets[it.key().toStdString()];
            auto &versionsMap = badgeSet.versions;

            for (auto versionIt = std::begin(versions); versionIt != std::end(versions);
                 ++versionIt) {
                std::string kkey = versionIt.key().toStdString();
                QJsonObject versionObj = versionIt.value().toObject();
                BadgeVersion v(std::move(versionObj));
                versionsMap.emplace(kkey, v);
            }
        }

        ch.loaded = true;
    });

    QString cheermoteURL = "https://api.twitch.tv/kraken/bits/actions?channel_id=" + roomID;

    twitchApiGet2(
        cheermoteURL, QThread::currentThread(), true, [this, roomID](const rapidjson::Document &d) {
            Resources::Channel &ch = this->channels[roomID];

            ParseCheermoteSets(ch.jsonCheermoteSets, d);

            for (auto &set : ch.jsonCheermoteSets) {
                CheermoteSet cheermoteSet;
                cheermoteSet.regex =
                    QRegularExpression("^" + set.prefix.toLower() + "([1-9][0-9]*)$");

                for (auto &tier : set.tiers) {
                    Cheermote cheermote;

                    cheermote.color = QColor(tier.color);
                    cheermote.minBits = tier.minBits;

                    // TODO(pajlada): We currently hardcode dark here :|
                    // We will continue to do so for now since we haven't had to
                    // solve that anywhere else
                    cheermote.emoteDataAnimated.image1x = tier.images["dark"]["animated"]["1"];
                    cheermote.emoteDataAnimated.image2x = tier.images["dark"]["animated"]["2"];
                    cheermote.emoteDataAnimated.image3x = tier.images["dark"]["animated"]["4"];

                    cheermote.emoteDataStatic.image1x = tier.images["dark"]["static"]["1"];
                    cheermote.emoteDataStatic.image2x = tier.images["dark"]["static"]["2"];
                    cheermote.emoteDataStatic.image3x = tier.images["dark"]["static"]["4"];

                    cheermoteSet.cheermotes.emplace_back(cheermote);
                }

                std::sort(cheermoteSet.cheermotes.begin(), cheermoteSet.cheermotes.end(),
                          [](const auto &lhs, const auto &rhs) {
                              return lhs.minBits < rhs.minBits;  //
                          });

                ch.cheermoteSets.emplace_back(cheermoteSet);
            }
        });
}

void Resources::loadDynamicTwitchBadges()
{
    static QString url("https://badges.twitch.tv/v1/badges/global/display?language=en");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());
    req.getJSON([this](QJsonObject &root) {
        QJsonObject sets = root.value("badge_sets").toObject();
        for (QJsonObject::iterator it = sets.begin(); it != sets.end(); ++it) {
            QJsonObject versions = it.value().toObject().value("versions").toObject();

            auto &badgeSet = this->badgeSets[it.key().toStdString()];
            auto &versionsMap = badgeSet.versions;

            for (auto versionIt = std::begin(versions); versionIt != std::end(versions);
                 ++versionIt) {
                std::string kkey = versionIt.key().toStdString();
                QJsonObject versionObj = versionIt.value().toObject();
                BadgeVersion v(std::move(versionObj));
                versionsMap.emplace(kkey, v);
            }
        }

        this->dynamicBadgesLoaded = true;
    });
}

void Resources::loadChatterinoBadges()
{
    this->chatterinoBadges.clear();

    static QString url("https://fourtf.com/chatterino/badges.json");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.getJSON([this](QJsonObject &root) {
        QJsonArray badgeVariants = root.value("badges").toArray();
        for (QJsonArray::iterator it = badgeVariants.begin(); it != badgeVariants.end(); ++it) {
            QJsonObject badgeVariant = it->toObject();
            const std::string badgeVariantTooltip =
                badgeVariant.value("tooltip").toString().toStdString();
            const QString &badgeVariantImageURL = badgeVariant.value("image").toString();

            auto badgeVariantPtr = std::make_shared<ChatterinoBadge>(
                badgeVariantTooltip, new Image(badgeVariantImageURL));

            QJsonArray badgeVariantUsers = badgeVariant.value("users").toArray();

            for (QJsonArray::iterator it = badgeVariantUsers.begin(); it != badgeVariantUsers.end();
                 ++it) {
                const std::string username = it->toString().toStdString();
                this->chatterinoBadges[username] =
                    std::shared_ptr<ChatterinoBadge>(badgeVariantPtr);
            }
        }
    });
}

}  // namespace chatterino
