#pragma once

#include "common/Channel.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/TitlebarButton.hpp"

#include <QPropertyAnimation>
#include <QWidget>

namespace chatterino {

class OverlayWindow : public QWidget
{
    Q_OBJECT
public:
    OverlayWindow(ChannelPtr channel, Split *split);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    Q_PROPERTY(double interactionProgress READ interactionProgress WRITE
                   setInteractionProgress)

    double interactionProgress() const;
    void setInteractionProgress(double progress);

    void startInteraction();
    void endInteraction();

    ChannelPtr channel_;
    ChannelView channelView_;

    bool moving_ = false;
    QPoint moveOrigin_;

    double interactionProgress_ = 0.0;
    QPropertyAnimation interactAnimation_;

    TitleBarButton closeButton_;
};

}  // namespace chatterino
