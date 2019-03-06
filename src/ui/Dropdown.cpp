#include "Dropdown.hpp"

#include <QMenu>

namespace chatterino::ui
{
    Dropdown::Dropdown(QWidget* parent)
        : Dropdown(new QMenu(parent), parent)
    {
    }

    Dropdown::Dropdown(QMenu* menu, QWidget* parent)
        : parent_(parent)
        , menu_(menu)
    {
        this->menu_->setAttribute(Qt::WA_DeleteOnClose);
    }

    void Dropdown::addSeperator()
    {
        assert(this->menu_);

        this->menu_->addSeparator();
    }

    void Dropdown::addItem(const QString& title, std::function<void()> onClick)
    {
        assert(this->menu_);

        this->menu_->addAction(title, std::move(onClick));
    }

    void Dropdown::addItem(
        const QString& title, bool value, std::function<void(bool)> onClick)
    {
        assert(this->menu_);

        auto action = new QAction(title);
        action->setCheckable(true);
        action->setChecked(value);
        QObject::connect(action, &QAction::toggled, std::move(onClick));
    }

    Dropdown Dropdown::addSubmenu(const QString& title)
    {
        assert(this->menu_);

        return {this->menu_->addMenu(title), this->parent_};
    }

    QWidget* Dropdown::releaseWidget()
    {
        return this->menu_.release();
    }
}  // namespace chatterino::ui
