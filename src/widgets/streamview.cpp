#include "streamview.hpp"

#include "channel.hpp"
#include "util/helpers.hpp"
#include "util/layoutcreator.hpp"
#include "widgets/split.hpp"

#ifdef USEWEBENGINE
#include <QtWebEngineWidgets>
#endif

namespace chatterino {
namespace widgets {
StreamView::StreamView(SharedChannel channel, QUrl url)
{
    util::LayoutCreator<StreamView> layoutCreator(this);

#ifdef USEWEBENGINE
    auto web = layoutCreator.emplace<QWebEngineView>(this).assign(&this->stream);
    web->setUrl(url);
    web->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
#endif

    //    QString uuid = CreateUUID();

    auto chat = layoutCreator.emplace<ChannelView>();
    chat->setFixedWidth(300);
    chat->setChannel(channel);

    this->layout()->setSpacing(0);
    this->layout()->setMargin(0);
}
}  // namespace widgets
}  // namespace chatterino
