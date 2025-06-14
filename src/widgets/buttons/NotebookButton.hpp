#pragma once

#include "widgets/buttons/Button.hpp"

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
    void paintContent(QPainter &painter) override;

    void themeChangedEvent() override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dragLeaveEvent(QDragLeaveEvent *) override;
    void dropEvent(QDropEvent *) override;

    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;

private:
    Notebook *parent_ = nullptr;
    QPoint mousePos_;
    Icon icon_ = None;
};

}  // namespace chatterino
