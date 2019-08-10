#include "StreamView.hpp"

#include "common/Channel.hpp"
#include "util/Helpers.hpp"
#include "util/LayoutCreator.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/splits/Split.hpp"

#ifdef USEWEBENGINE
#    include <QtWebEngineWidgets>
#endif

namespace chatterino {

StreamView::StreamView(ChannelPtr channel, const QUrl &url)
{
    LayoutCreator<StreamView> layoutCreator(this);

#ifdef USEWEBENGINE
    auto web =
        layoutCreator.emplace<QWebEngineView>(this).assign(&this->stream);
    web->setUrl(url);
    web->settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled,
                                  true);
#endif

    auto chat = layoutCreator.emplace<ChannelView>();
    chat->setFixedWidth(300);
    chat->setChannel(channel);

    this->layout()->setSpacing(0);
    this->layout()->setMargin(0);
}

}  // namespace chatterino
