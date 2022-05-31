#pragma once

#include <QStringList>

namespace chatterino {

/// Sample messages coming from IRC

const QStringList &getSampleCheerMessages();
const QStringList &getSampleSubMessages();
const QStringList &getSampleMiscMessages();
const QStringList &getSampleEmoteTestMessages();

/// Channel point reward tests

const QString &getSampleChannelRewardMessage();
const QString &getSampleChannelRewardMessage2();
const QString &getSampleChannelRewardIRCMessage();

/// Links

const QStringList &getSampleLinkMessages();

}  // namespace chatterino
