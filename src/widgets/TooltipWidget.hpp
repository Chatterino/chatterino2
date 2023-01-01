#pragma once

#include "widgets/BaseWindow.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QLabel>
#include <QWidget>

namespace chatterino {

class Image;
using ImagePtr = std::shared_ptr<Image>;

class TooltipWidget : public BaseWindow
{
    Q_OBJECT

public:
    static TooltipWidget *instance();

    TooltipWidget(BaseWidget *parent = nullptr);
    ~TooltipWidget() override = default;

    void setText(QString text);
    void setWordWrap(bool wrap);
    void clearImage();
    void setImage(ImagePtr image);
    void setImageScale(int w, int h);

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;
    void changeEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void themeChangedEvent() override;
    void scaleChangedEvent(float) override;
    void paintEvent(QPaintEvent *) override;

private:
    void updateFont();

    // used by WindowManager::gifRepaintRequested signal to progress frames when tooltip image is animated
    bool refreshPixmap();

    // set to true when tooltip image did not finish loading yet (pixmapOrLoad returned false)
    bool attemptRefresh{false};

    ImagePtr image_ = nullptr;
    int customImgWidth = 0;
    int customImgHeight = 0;
    QLabel *displayImage_;
    QLabel *displayText_;
    pajlada::Signals::SignalHolder connections_;
};

}  // namespace chatterino
