// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "MessageBuilding.hpp"

#include "messages/Emote.hpp"

#include <QJsonDocument>
#include <QJsonObject>

namespace {

using namespace Qt::Literals;

std::optional<QJsonDocument> tryReadJsonFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
    {
        return std::nullopt;
    }

    QJsonParseError e;
    auto doc = QJsonDocument::fromJson(file.readAll(), &e);
    if (e.error != QJsonParseError::NoError)
    {
        return std::nullopt;
    }

    return doc;
}

QJsonDocument readJsonFile(const QString &path)
{
    auto opt = tryReadJsonFile(path);
    if (!opt)
    {
        _exit(1);
    }
    return *opt;
}

}  // namespace

namespace chatterino::bench {

MockMessageApplication::MockMessageApplication()
    : highlights(this->settings, &this->accounts)
{
}

MessageBenchmark::MessageBenchmark(QString name)
    : name(std::move(name))
    , chan(std::make_shared<TwitchChannel>(this->name))
{
    const auto seventvEmotes =
        tryReadJsonFile(u":/bench/seventvemotes-%1.json"_s.arg(this->name));
    const auto bttvEmotes =
        tryReadJsonFile(u":/bench/bttvemotes-%1.json"_s.arg(this->name));
    const auto ffzEmotes =
        tryReadJsonFile(u":/bench/ffzemotes-%1.json"_s.arg(this->name));

    if (seventvEmotes)
    {
        this->chan->setSeventvEmotes(std::make_shared<const EmoteMap>(
            seventv::detail::parseEmotes(seventvEmotes->object()["emote_set"_L1]
                                             .toObject()["emotes"_L1]
                                             .toArray(),
                                         false)));
    }

    if (bttvEmotes)
    {
        this->chan->setBttvEmotes(
            std::make_shared<const EmoteMap>(bttv::detail::parseChannelEmotes(
                bttvEmotes->object(), this->name)));
    }

    if (ffzEmotes)
    {
        this->chan->setFfzEmotes(std::make_shared<const EmoteMap>(
            ffz::detail::parseChannelEmotes(ffzEmotes->object())));
    }

    this->messages =
        readJsonFile(u":/bench/recentmessages-%1.json"_s.arg(this->name));
}

}  // namespace chatterino::bench
