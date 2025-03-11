#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QWidget>

namespace chatterino {

class ColorInput : public QWidget
{
    Q_OBJECT

public:
    ColorInput(QColor color, QWidget *parent = nullptr);

    QColor color() const;

Q_SIGNALS:
    void colorChanged(QColor color);

public Q_SLOTS:
    void setColor(QColor color);

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

    QLabel hexLabel_;
    QLineEdit hexInput_;
    QRegularExpressionValidator hexValidator_;

    QGridLayout layout_;

    void updateComponents();
    void updateHex();

    void emitUpdate();
};

}  // namespace chatterino
