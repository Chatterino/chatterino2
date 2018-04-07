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
    QWebEngineView *stream;
};

}  // namespace widgets
}  // namespace chatterino
