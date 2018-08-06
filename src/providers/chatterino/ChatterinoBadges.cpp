#include "ChatterinoBadges.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QThread>
#include "common/NetworkRequest.hpp"

namespace chatterino {

ChatterinoBadges::ChatterinoBadges()
{
}

boost::optional<EmotePtr> ChatterinoBadges::getBadge(const UserName &username)
{
    return this->badges.access()->get(username);
}

void ChatterinoBadges::loadChatterinoBadges()
{
    static QString url("https://fourtf.com/chatterino/badges.json");

    NetworkRequest req(url);
    req.setCaller(QThread::currentThread());

    req.onSuccess([this](auto result) {
        auto jsonRoot = result.parseJson();
        auto badges = this->badges.access();
        auto replacement = badges->makeReplacment();

        for (auto jsonBadge_ : jsonRoot.value("badges").toArray()) {
            auto jsonBadge = jsonBadge_.toObject();

            auto emote = Emote{
                EmoteName{}, ImageSet{Url{jsonBadge.value("image").toString()}},
                Tooltip{jsonBadge.value("tooltip").toString()}, Url{}};

            for (auto jsonUser : jsonBadge.value("users").toArray()) {
                replacement.add(UserName{jsonUser.toString()},
                                std::move(emote));
            }
        }

        replacement.apply();
        return Success;
    });

    req.execute();
}

}  // namespace chatterino
