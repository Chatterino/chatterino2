// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QStringList>

namespace chatterino {

/// Sample messages coming from IRC

const QStringList &getSampleCheerMessages();
const QStringList &getSampleSubMessages();
const QStringList &getSampleMiscMessages();
const QStringList &getSampleEmoteTestMessages();

/// Channel point reward tests

QByteArray getSampleChannelRewardMessage();
QByteArray getSampleChannelRewardMessage2();
const QString &getSampleChannelRewardIRCMessage();

/// Links

const QStringList &getSampleLinkMessages();

}  // namespace chatterino
