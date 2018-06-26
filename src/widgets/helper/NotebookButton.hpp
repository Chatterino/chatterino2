#pragma once

#include "RippleEffectButton.hpp"

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
    virtual void themeRefreshEvent() override;
    virtual void paintEvent(QPaintEvent *) override;
    virtual void mouseReleaseEvent(QMouseEvent *) override;
    virtual void dragEnterEvent(QDragEnterEvent *) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *) override;
    virtual void dropEvent(QDropEvent *) override;

signals:
    void clicked();

private:
    QPoint mousePos_;
};

}  // namespace widgets
}  // namespace chatterino
