// SPDX-FileCopyrightText: 2024 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "providers/pronouns/alejo/PronounsAlejoApi.hpp"

#include "common/network/NetworkRequest.hpp"
#include "common/network/NetworkResult.hpp"
#include "common/QLogging.hpp"
#include "providers/pronouns/UserPronouns.hpp"

#include <QStringBuilder>

#include <mutex>
#include <unordered_map>

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
const auto &LOG = chatterinoPronouns;

constexpr QStringView API_URL = u"https://api.pronouns.alejo.io/v1";
constexpr QStringView API_USERS_ENDPOINT = u"/users";
constexpr QStringView API_PRONOUNS_ENDPOINT = u"/pronouns";

}  // namespace

namespace chatterino::pronouns {

AlejoApi::AlejoApi()
{
    this->loadAvailablePronouns();
}

void AlejoApi::fetch(
    const QString &username,
    const std::function<void(std::optional<UserPronouns>)> &onDone)
{
    {
        std::shared_lock lock(this->mutex);
        if (this->pronouns.empty())
        {
            // Pronoun list not available yet, fail and try again next time.
            onDone({});
            return;
        }
    }

    qCDebug(LOG) << "Fetching pronouns from alejo.io for" << username;

    QString endpoint = API_URL % API_USERS_ENDPOINT % "/" % username;

    NetworkRequest(endpoint)
        .concurrent()
        .onSuccess([this, username, onDone](const auto &result) {
            auto object = result.parseJson();
            auto parsed = this->parsePronoun(object);
            onDone({parsed});
        })
        .onError([onDone, username](const auto &result) {
            auto status = result.status();
            if (status.has_value() && status == 404)
            {
                // Alejo returns 404 if the user has no pronouns set.
                // Return unspecified.
                onDone({UserPronouns()});
                return;
            }
            qCWarning(LOG) << "alejo.io returned " << status.value_or(-1)
                           << " when fetching pronouns for " << username;
            onDone({});
        })
        .execute();
}

void AlejoApi::loadAvailablePronouns()
{
    qCDebug(LOG) << "Fetching available pronouns for alejo.io";

    QString endpoint = API_URL % API_PRONOUNS_ENDPOINT;

    NetworkRequest(endpoint)
        .concurrent()
        .onSuccess([this](const auto &result) {
            auto root = result.parseJson();
            if (root.isEmpty())
            {
                return;
            }

            std::unordered_map<QString, PronounEntry> newPronouns;

            for (auto it = root.begin(); it != root.end(); ++it)
            {
                const auto &pronounId = it.key();
                const auto &pronounObj = it.value().toObject();

                const auto &subject = pronounObj["subject"].toString();
                const auto &object = pronounObj["object"].toString();
                const auto &singular = pronounObj["singular"].toBool();

                if (subject.isEmpty() || object.isEmpty())
                {
                    qCWarning(LOG) << "Pronoun" << pronounId
                                   << "was malformed:" << pronounObj;
                    continue;
                }
                newPronouns[pronounId] =
                    PronounEntry{subject, object, singular};
            }

            {
                std::unique_lock lock(this->mutex);
                this->pronouns = newPronouns;
            }
        })
        .onError([](const NetworkResult &result) {
            qCWarning(LOG) << "Failed to load pronouns from alejo.io"
                           << result.formatError();
        })
        .execute();
}

UserPronouns AlejoApi::parsePronoun(const QJsonObject &object)
{
    const QJsonValue pronounMain = object["pronoun_id"];
    const QJsonValue pronounAlt = object["alt_pronoun_id"];

    if (!pronounMain.isString())
    {
        return {};
    }

    std::shared_lock lock(this->mutex);

    const auto iterMain = this->pronouns.find(pronounMain.toString());
    if (iterMain == this->pronouns.end())
    {
        return {};
    }

    if (!pronounAlt.isString())
    {
        if (iterMain->second.singular)
        {
            return {iterMain->second.subject};
        }
        return {iterMain->second.subject + "/" + iterMain->second.object};
    }

    const auto iterAlt = this->pronouns.find(pronounAlt.toString());
    if (iterAlt != this->pronouns.end())
    {
        return {iterMain->second.subject + "/" + iterAlt->second.subject};
    }

    return {};
}
}  // namespace chatterino::pronouns
