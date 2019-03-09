#include "Dropdown.hpp"

#include <QMenu>

namespace chatterino::ui
{
    Dropdown::Dropdown(QWidget* parent)
        : owning_(new QMenu(parent))
        , menu_(this->owning_.get())
    {
        this->menu_->setAttribute(Qt::WA_DeleteOnClose);
    }

    Dropdown::Dropdown(QMenu* menu)
        : menu_(menu)
    {
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

        this->menu_->addAction(action);
    }

    Dropdown Dropdown::addSubmenu(const QString& title)
    {
        assert(this->menu_);

        return {this->menu_->addMenu(title)};
    }

    QWidget* Dropdown::releaseWidget()
    {
        assert(this->owning_);

        return this->owning_.release();
    }
}  // namespace chatterino::ui
