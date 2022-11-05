#pragma once

#include "common/Channel.hpp"

#include <QString>
#include <QStringList>

namespace chatterino::commands {

QString emoteOnly(const QStringList &words, ChannelPtr channel);
QString emoteOnlyOff(const QStringList &words, ChannelPtr channel);

QString subscribers(const QStringList &words, ChannelPtr channel);
QString subscribersOff(const QStringList &words, ChannelPtr channel);

QString slow(const QStringList &words, ChannelPtr channel);
QString slowOff(const QStringList &words, ChannelPtr channel);

QString followers(const QStringList &words, ChannelPtr channel);
QString followersOff(const QStringList &words, ChannelPtr channel);

QString uniqueChat(const QStringList &words, ChannelPtr channel);
QString uniqueChatOff(const QStringList &words, ChannelPtr channel);

}  // namespace chatterino::commands
