#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"

#include <algorithm>

namespace chatterino {

/*static*/ std::mutex PronounsAlejoApi::mutex = {};
/*static*/ std::optional<std::unordered_map<std::string, std::string>>
    PronounsAlejoApi::pronounsFromId = {};

/*static*/ UserPronouns PronounsAlejoApi::parse(QJsonObject object)
{
    if (!PronounsAlejoApi::pronounsFromId.has_value())
    {
        return {};
    }

    auto pronoun = object["pronoun_id"];

    if (!pronoun.isString())
    {
        return {};
    }

    auto pronounQStr = pronoun.toString();
    auto pronounStr = pronounQStr.toStdString();
    auto iter = PronounsAlejoApi::pronounsFromId->find(pronounStr);
    if (iter != PronounsAlejoApi::pronounsFromId->end())
    {
        return {iter->second};
    }
    else
    {
        return {};
    }
}

PronounsAlejoApi::PronounsAlejoApi()
{
    std::lock_guard<std::mutex> lock(PronounsAlejoApi::mutex);
    if (!PronounsAlejoApi::pronounsFromId)
    {
        qCDebug(chatterinoPronouns)
            << "Fetching available pronouns for alejo.io";
        NetworkRequest(PronounsAlejoApi::API_URL +
                       PronounsAlejoApi::API_PRONOUNS)
            .concurrent()
            .cache()
            .onSuccess([](auto result) {
                auto object = result.parseJson();
                if (object.isEmpty())
                {
                    return;
                }

                std::lock_guard<std::mutex> lock(PronounsAlejoApi::mutex);
                PronounsAlejoApi::pronounsFromId = {
                    std::unordered_map<std::string, std::string>()};
                for (auto const &pronounId : object.keys())
                {
                    if (!object[pronounId].isObject())
                    {
                        continue;
                    };

                    auto pronounObj = object[pronounId].toObject();

                    if (!pronounObj["subject"].isString())
                    {
                        continue;
                    }

                    std::string pronouns =
                        pronounObj["subject"].toString().toStdString();

                    auto singular = pronounObj["singular"];
                    if (singular.isBool() && !singular.toBool() &&
                        pronounObj["object"].isString())
                    {
                        pronouns +=
                            "/" + pronounObj["object"].toString().toStdString();
                    }

                    // Make lowercase
                    std::transform(pronouns.begin(), pronouns.end(),
                                   pronouns.begin(), [](unsigned char c) {
                                       return std::tolower(c);
                                   });
                    (*PronounsAlejoApi::pronounsFromId)
                        .insert_or_assign(pronounId.toStdString(), pronouns);
                }
            })
            .execute();
    }
}

void PronounsAlejoApi::fetch(std::vector<PronounUser> users,
                             IPronounsApi::RequestT::CallbackT &&onDone)
{
    if (users.empty())
    {
        onDone(IPronounsApi::RequestT::ResultsT());
        return;
    }

    auto request = std::make_shared<IPronounsApi::RequestT>(users.size(),
                                                            std::move(onDone));

    bool havePronounList{true};
    {
        std::lock_guard<std::mutex> lock(PronounsAlejoApi::mutex);
        havePronounList = PronounsAlejoApi::pronounsFromId.has_value();
    }  // unlock mutex

    if (!havePronounList)
    {
        // Pronoun list not available yet, just fail and try again next time.
        for (auto const &user : users)
        {
            request->finishRequest({user.username, {}});
        }
        return;
    }

    for (auto const &user : users)
    {
        NetworkRequest(PronounsAlejoApi::API_URL + PronounsAlejoApi::API_USERS +
                       "/" + user.username)
            .concurrent()
            .onSuccess([request, user](auto result) {
                if (!request)
                    return;

                auto object = result.parseJson();

                // return (user.login, pronouns)
                // pronouns may be std::nullopt if no pronouns are set.
                request->finishRequest(
                    {user.username, {PronounsAlejoApi::parse(object)}});
            })
            .onError([request, user](auto error) {
                if (!request)
                    return;

                // error => return (user.login, std::nullopt)
                request->finishRequest({user.username, {}});
            })
            .execute();
    }
}

std::size_t PronounsAlejoApi::maxBatchSize()
{
    return 10;
}

}  // namespace chatterino
