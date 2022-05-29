#pragma once

#include <QStringList>

namespace chatterino {

/// Sample Messages

const QStringList &getSampleCheerMessages();
const QStringList &getSampleSubMessages();
const QStringList &getSampleMiscMessages();
const QStringList &getSampleEmoteTestMessages();

/// channel point reward tests

const QString &getSampleChannelRewardMessage();
const QString &getSampleChannelRewardMessage2();
const QString &getSampleChannelRewardIRCMessage();

/// Links

const QStringList &getSampleLinkMessages();

}  // namespace chatterino
