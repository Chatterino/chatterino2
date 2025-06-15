#pragma once

#include "widgets/buttons/Button.hpp"

#include <QWidget>

namespace chatterino {

class Notebook;

class NotebookButton : public Button
{
    Q_OBJECT

public:
    enum class Type : std::uint8_t {
        Plus,
    };

    NotebookButton(Type type_, Notebook *parent);

protected:
    void paintContent(QPainter &painter) override;

    void themeChangedEvent() override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;

    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;

private:
    Notebook *parent_ = nullptr;
    QPoint mousePos_;
    Type type = Type::Plus;
};

}  // namespace chatterino
