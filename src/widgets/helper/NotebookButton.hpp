#pragma once

#include "widgets/helper/Button.hpp"

#include <QWidget>

namespace chatterino {

class Notebook;

class NotebookButton : public Button
{
    Q_OBJECT

public:
    enum Icon { None, Plus, User, Settings };

    explicit NotebookButton(Notebook *parent);

    void setIcon(Icon icon);
    Icon getIcon() const;

protected:
    void themeChangedEvent() override;
    void paintEvent(QPaintEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;

    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;

Q_SIGNALS:
    void leftClicked();

private:
    Notebook *parent_ = nullptr;
    QPoint mousePos_;
    Icon icon_ = None;
};

}  // namespace chatterino
