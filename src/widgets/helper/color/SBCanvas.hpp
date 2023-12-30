#pragma once

#include <QWidget>

namespace chatterino {

class SBCanvas : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int saturation READ saturation NOTIFY saturationChanged)
    Q_PROPERTY(int brightness READ brightness NOTIFY brightnessChanged)

public:
    SBCanvas(QColor color = {}, QWidget *parent = nullptr);

    void updateColor(const QColor &color);
    void setHue(int hue);
    QSize sizeHint() const override;

signals:
    void saturationChanged(int saturation) const;
    void brightnessChanged(int brightness) const;

public slots:
    int saturation() const;
    int brightness() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int hue_ = 0;
    int saturation_ = 0;
    int brightness_ = 0;

    QPixmap gradientPixmap_;

    bool trackingMouseEvents_ = false;

    void updatePixmap();
    int xPosToSaturation(int xPos) const;
    int yPosToBrightness(int yPos) const;

    void updateFromEvent(QMouseEvent *event);

    void setSaturation(int saturation);
    void setBrightness(int brightness);
};

}  // namespace chatterino
