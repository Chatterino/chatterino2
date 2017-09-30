#pragma once

#include "channel.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/limitedqueuesnapshot.hpp"
#include "messages/word.hpp"
#include "widgets/accountpopup.hpp"
#include "widgets/basewidget.hpp"
#include "widgets/rippleeffectlabel.hpp"
#include "widgets/scrollbar.hpp"

#include <QPaintEvent>
#include <QScroller>
#include <QWheelEvent>
#include <QWidget>
#include <QtWebEngineWidgets/QWebEngineView>

#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

class ChannelView : public BaseWidget
{
    Q_OBJECT

public:
    explicit ChannelView(WindowManager &windowManager, BaseWidget *parent = 0);
    ~ChannelView();

    void setChannel(std::shared_ptr<Channel> channel);
    void addMessage(messages::SharedMessage &message);

    void clearMessages();

private:
    struct GifEmoteData {
        messages::LazyLoadedImage *image;
        QRect rect;
    };

    WindowManager &windowManager;

    void detachChannel();

    std::shared_ptr<Channel> channel;

    std::vector<GifEmoteData> gifEmotes;

    AccountPopupWidget userPopupWidget;

    boost::signals2::connection messageAppendedConnection;
    QWebEngineView web;
    QVBoxLayout vbox;
};

}  // namespace widgets
}  // namespace chatterino
