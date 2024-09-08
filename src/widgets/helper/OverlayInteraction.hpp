#pragma once

#include "widgets/helper/TitlebarButton.hpp"

#include <QPropertyAnimation>
#include <QWidget>

class QGridLayout;

namespace chatterino {

class OverlayWindow;
class OverlayInteraction : public QWidget
{
    Q_OBJECT
public:
    OverlayInteraction(OverlayWindow *parent);

    void attach(QGridLayout *layout);

    QWidget *closeButton();

    void startInteraction();
    void endInteraction();

    bool isInteracting() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Q_PROPERTY(double interactionProgress READ interactionProgress WRITE
                   setInteractionProgress)

    TitleBarButton closeButton_;

    double interactionProgress() const;
    void setInteractionProgress(double progress);

    bool interacting_ = false;
    double interactionProgress_ = 0.0;
    QPropertyAnimation interactAnimation_;

    OverlayWindow *window_;
};

}  // namespace chatterino
