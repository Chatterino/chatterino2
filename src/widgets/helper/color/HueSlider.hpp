#pragma once

#include <QWidget>

namespace chatterino {

class HueSlider : public QWidget
{
    Q_OBJECT

public:
    HueSlider(QColor color, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    int hue() const;

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
    QColor color_;

    QPixmap gradientPixmap_;

    bool trackingMouseEvents_ = false;

    void updatePixmap();
    int xPosToHue(int xPos) const;

    void updateFromEvent(QMouseEvent *event);

    void setHue(int hue);
};

}  // namespace chatterino
