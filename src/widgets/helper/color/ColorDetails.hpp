#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>

namespace chatterino {

class ColorDetails : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)

public:
    ColorDetails(QColor color = {}, QWidget *parent = nullptr);

    QColor color() const;

signals:
    void colorChanged(QColor color);

public slots:
    void setColor(const QColor &color);

private:
    QColor currentColor_;

    struct Component {
        QLabel lbl;
        QSpinBox box;
        int value = -1;
    };

    Component red_;
    Component green_;
    Component blue_;
    Component alpha_;

    QLabel cssLabel_;
    QLineEdit cssInput_;
    QRegularExpressionValidator cssValidator_;

    QGridLayout layout_;

    void updateComponents();
    void updateCss();
};

}  // namespace chatterino
