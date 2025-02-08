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

    QDesktopServices::openUrl(
        QUrl{scheme % u"https://twitch.tv/" % channelName});
}

}  // namespace chatterino
