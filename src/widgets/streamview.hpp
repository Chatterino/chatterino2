#pragma once

#include <QUrl>
#include <QWidget>

#include <memory>

class QWebEngineView;

namespace chatterino {

class Channel;

namespace widgets {

class StreamView : public QWidget
{
public:
    StreamView(std::shared_ptr<Channel> channel, const QUrl &url);

private:
#ifdef USEWEBENGINE
    QWebEngineView *stream;
#endif
};

}  // namespace widgets
}  // namespace chatterino
