#pragma once

#include <QAbstractButton>

namespace chatterino {

class ColorButton : public QAbstractButton
{
    Q_OBJECT

public:
    ColorButton(QColor color, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    QColor color() const;

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public Q_SLOTS:
    void setColor(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QColor currentColor_;

    QPixmap checkerboardCache_;
    bool checkerboardCacheValid_ = false;
};

}  // namespace chatterino
