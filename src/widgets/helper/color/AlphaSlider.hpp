#pragma once

#include <QWidget>

namespace chatterino {

class AlphaSlider : public QWidget
{
    Q_OBJECT

public:
    AlphaSlider(QColor color, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    int alpha() const;

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
    int alpha_ = 255;
    QColor color_;

    QPixmap cachedPixmap_;

    bool trackingMouseEvents_ = false;

    void updatePixmap();
    int xPosToAlpha(int xPos) const;

    void updateFromEvent(QMouseEvent *event);

    void setAlpha(int alpha);
};

}  // namespace chatterino
