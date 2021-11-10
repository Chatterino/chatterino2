#include "PronounsBadges.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <map>
#include <shared_mutex>
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

void PronounsBadges::initialize(Settings &settings, Paths &paths)
{
    this->loadPronouns();
    this->enabled_ = settings.showBadgesPronouns;
    this->settingListener_.addSetting(settings.showBadgesPronouns);
    this->settingListener_.setCB([this] {
        auto settings = getSettings();
        this->enabled_ = settings->showBadgesPronouns;
    });
}

boost::optional<QString> PronounsBadges::getPronouns(const UserId &id,
                                                     const QString &userName)
{
    if (!this->enabled_)
    {
        return boost::none;
    }

    std::shared_lock lock(this->mutex_);

    auto it = this->userPronounsMap_.find(id.string);
    if (it != this->userPronounsMap_.end())
    {
        return it->second;
    }
    else
    {
        this->userPronounsMap_[id.string] = boost::none;

        QUrl url("https://pronouns.alejo.io/api/users/" + userName);

        NetworkRequest(url)
            .onSuccess([this, id, userName](auto result) -> Outcome {
                std::unique_lock lock(this->mutex_);
                auto jsonRoot = result.parseJsonArray();
                for (const auto &jsonUser_ : jsonRoot)
                {
                    auto jsonUser = jsonUser_.toObject();
                    auto pronounId = jsonUser.value("pronoun_id").toString();

                    auto it = this->pronounsMap_.find(pronounId);
                    if (it != this->pronounsMap_.end())
                    {
                        this->userPronounsMap_[id.string] = it->second;
                    }
                }

                return Success;
            })
            .execute();
    }
    return boost::none;
}

void PronounsBadges::loadPronouns()
{
    static QUrl url("https://pronouns.alejo.io/api/pronouns");

    NetworkRequest(url)
        .onSuccess([this](auto result) -> Outcome {
            std::unique_lock lock(this->mutex_);

            auto jsonRoot = result.parseJsonArray();
            for (const auto &jsonPronouns_ : jsonRoot)
            {
                auto jsonPronouns = jsonPronouns_.toObject();
                auto name = jsonPronouns.value("name").toString();
                auto display = jsonPronouns.value("display").toString();

                this->pronounsMap_[name] = display;
            }

            return Success;
        })
        .execute();
}

}  // namespace chatterino
