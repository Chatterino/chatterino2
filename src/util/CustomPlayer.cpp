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

    QString encodedTwitchUrl = QString::fromUtf8(
        QUrl::toPercentEncoding(u"https://www.twitch.tv/" % channelName));
    QDesktopServices::openUrl(QUrl{scheme % encodedTwitchUrl});
}

}  // namespace chatterino
