#pragma once

#include "rippleeffectbutton.hpp"

#include <QWidget>

namespace chatterino {
namespace widgets {

class NotebookButton : public RippleEffectButton
{
    Q_OBJECT

public:
    static const int IconPlus = 0;
    static const int IconUser = 1;
    static const int IconSettings = 2;

    int icon = 0;

    NotebookButton(BaseWidget *parent);

protected:
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void clicked();

private:
    QPoint mousePos;
};

}  // namespace widgets
}  // namespace chatterino
