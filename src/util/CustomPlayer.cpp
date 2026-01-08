// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "util/CustomPlayer.hpp"

#include "common/QLogging.hpp"
#include "singletons/Settings.hpp"

#include <QDesktopServices>
#include <QStringBuilder>
#include <QStringView>
#include <QUrl>

namespace chatterino {

void openInCustomPlayer(QStringView channelName)
{
    QString scheme = getSettings()->customURIScheme.getValue();
    if (scheme.isEmpty())
    {
        qCWarning(chatterinoApp)
            << "Can't open" << channelName
            << "in custom player because no URI scheme is set";
        return;
    }

    auto twitchUrl = "https://www.twitch.tv/" + channelName;
    auto encodedTwitchUrl = QUrl::toPercentEncoding(twitchUrl);
    QDesktopServices::openUrl(QUrl(QStringLiteral("%1://%2").arg(
        scheme, QString::fromUtf8(encodedTwitchUrl))));
}

}  // namespace chatterino
