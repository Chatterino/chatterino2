#pragma once

#include "RippleEffectButton.hpp"

#include <QWidget>

namespace chatterino {

class Notebook;

class NotebookButton : public RippleEffectButton
{
    Q_OBJECT

public:
    enum Icon { None, Plus, User, Settings };

    explicit NotebookButton(Notebook *parent);

    void setIcon(Icon icon_);
    Icon getIcon() const;

protected:
    virtual void themeRefreshEvent() override;
    virtual void paintEvent(QPaintEvent *) override;
    virtual void mouseReleaseEvent(QMouseEvent *) override;
    virtual void dragEnterEvent(QDragEnterEvent *) override;
    virtual void dragLeaveEvent(QDragLeaveEvent *) override;
    virtual void dropEvent(QDropEvent *) override;

    virtual void hideEvent(QHideEvent *) override;
    virtual void showEvent(QShowEvent *) override;

signals:
    void clicked();

private:
    Notebook *parent_ = nullptr;
    QPoint mousePos_;
    Icon icon_ = None;
};

}  // namespace chatterino
