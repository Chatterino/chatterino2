#pragma once

#include <QWidget>

namespace chatterino {

/// 2D canvas for saturation (x-axis) and brightness (y-axis)
class SBCanvas : public QWidget
{
    Q_OBJECT

public:
    SBCanvas(QColor color, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    int saturation() const;
    int brightness() const;

Q_SIGNALS:
    void colorChanged(QColor color) const;

public Q_SLOTS:
    void setColor(QColor color);

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
    QColor color_;

    QPixmap gradientPixmap_;

    bool trackingMouseEvents_ = false;

    void updatePixmap();
    int xPosToSaturation(int xPos) const;
    int yPosToBrightness(int yPos) const;

    void updateFromEvent(QMouseEvent *event);

    [[nodiscard]] bool setSaturation(int saturation);
    [[nodiscard]] bool setBrightness(int brightness);

    void emitUpdatedColor();
};

}  // namespace chatterino
