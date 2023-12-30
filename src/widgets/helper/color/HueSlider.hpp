#pragma once

#include <QWidget>

namespace chatterino {

class HueSlider : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int hue READ hue NOTIFY hueChanged)

public:
    HueSlider(QColor color = {}, QWidget *parent = nullptr);

    void updateColor(const QColor &color);
    QSize sizeHint() const override;

    int hue() const;

signals:
    void hueChanged(int hue) const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int hue_ = 0;

    QPixmap gradientPixmap_;

    bool trackingMouseEvents_ = false;

    void updatePixmap();
    int xPosToHue(int xPos) const;

    void updateFromEvent(QMouseEvent *event);

    void setHue(int hue);
};

}  // namespace chatterino
