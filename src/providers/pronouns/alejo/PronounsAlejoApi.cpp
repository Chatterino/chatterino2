#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "providers/pronouns/UserPronouns.hpp"

namespace chatterino::pronouns {

UserPronouns AlejoApi::parse(const QJsonObject &object)
{
    if (!this->pronounsFromId.has_value())
    {
        return {};
    }

    auto pronoun = object["pronoun_id"];

    if (!pronoun.isString())
    {
        return {};
    }

    auto pronounStr = pronoun.toString();
    std::shared_lock lock(this->mutex);
    auto iter = this->pronounsFromId->find(pronounStr);
    if (iter != this->pronounsFromId->end())
    {
        return {iter->second};
    }
    return {};
}

AlejoApi::AlejoApi()
{
    std::shared_lock lock(this->mutex);
    if (this->pronounsFromId)
    {
        return;
    }

    qCDebug(chatterinoPronouns) << "Fetching available pronouns for alejo.io";
    NetworkRequest(AlejoApi::API_URL + AlejoApi::API_PRONOUNS)
        .concurrent()
        .onSuccess([this](const auto &result) {
            auto object = result.parseJson();
            if (object.isEmpty())
            {
                return;
            }

            std::unique_lock lock(this->mutex);
            this->pronounsFromId = {std::unordered_map<QString, QString>()};
            for (auto const &pronounId : object.keys())
            {
                if (!object[pronounId].isObject())
                {
                    continue;
                };

                const auto pronounObj = object[pronounId].toObject();

                if (!pronounObj["subject"].isString())
                {
                    continue;
                }

                QString pronouns = pronounObj["subject"].toString();

                auto singular = pronounObj["singular"];
                if (singular.isBool() && !singular.toBool() &&
                    pronounObj["object"].isString())
                {
                    pronouns += "/" + pronounObj["object"].toString();
                }

                this->pronounsFromId->insert_or_assign(pronounId,
                                                       pronouns.toLower());
            }
        })
        .execute();
}

void AlejoApi::fetch(const QString &username,
                     std::function<void(std::optional<UserPronouns>)> onDone)
{
    bool havePronounList{true};
    {
        std::shared_lock lock(this->mutex);
        havePronounList = this->pronounsFromId.has_value();
    }  // unlock mutex

    if (!havePronounList)
    {
        // Pronoun list not available yet, just fail and try again next time.
        onDone({});
        return;
    }

    NetworkRequest(AlejoApi::API_URL + AlejoApi::API_USERS + "/" + username)
        .concurrent()
        .onSuccess([this, username, onDone](const auto &result) {
            auto object = result.parseJson();
            auto parsed = this->parse(object);
            onDone({parsed});
        })
        .onError([onDone, username](auto result) {
            auto status = result.status();
            if (status.has_value() && status == 404)
            {
                // Alejo returns 404 if the user has no pronouns set.
                // Return unspecified.
                onDone({UserPronouns()});
                return;
            }
            qCWarning(chatterinoPronouns)
                << "alejo.io returned " << status.value_or(-1)
                << " when fetching pronouns for " << username;
            onDone({});
        })
        .execute();
}

}  // namespace chatterino::pronouns
