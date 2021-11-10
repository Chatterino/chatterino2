#include "PronounsBadges.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <map>
#include <shared_mutex>
#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"

namespace chatterino {

void PronounsBadges::initialize(Settings &settings, Paths &paths)
{
    this->loadPronouns();
}

boost::optional<QString> PronounsBadges::getPronouns(const UserId &id,
                                                     const QString &userName)
{
    std::shared_lock lock(this->mutex_);

    auto it = this->userPronounsMap.find(id.string);
    if (it != this->userPronounsMap.end())
    {
        return it->second;
    }
    else
    {
        this->userPronounsMap[id.string] = boost::none;

        QUrl url("https://pronouns.alejo.io/api/users/" + userName);

        NetworkRequest(url)
            .onSuccess([this, id, userName](auto result) -> Outcome {
                std::unique_lock lock(this->mutex_);
                auto jsonRoot = result.parseJsonArray();
                for (const auto &jsonUser_ : jsonRoot)
                {
                    auto jsonUser = jsonUser_.toObject();
                    auto pronounId = jsonUser.value("pronoun_id").toString();

                    auto it = this->pronounsMap.find(pronounId);
                    if (it != this->pronounsMap.end())
                    {
                        this->userPronounsMap[id.string] = it->second;
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

                this->pronounsMap[name] = display;
            }

            return Success;
        })
        .execute();
}

}  // namespace chatterino
