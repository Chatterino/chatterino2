#pragma once

#include <QWidget>

namespace chatterino {

class AlphaSlider : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int alpha READ alpha NOTIFY alphaChanged)

public:
    AlphaSlider(QColor color = {}, QWidget *parent = nullptr);

    void updateColor(const QColor &color);
    QSize sizeHint() const override;

    int alpha() const;

signals:
    void alphaChanged(int hue) const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    int alpha_ = 255;
    QColor baseColor_;

    QPixmap cachedPixmap_;

    bool trackingMouseEvents_ = false;

    void updatePixmap();
    int xPosToAlpha(int xPos) const;

    void updateFromEvent(QMouseEvent *event);

    void setAlpha(int alpha);
};

}  // namespace chatterino
