#pragma once

#include <QString>
#include <functional>
#include <memory>

class QMenu;
class QWidget;

namespace chatterino::ui
{
    class Dropdown
    {
    public:
        explicit Dropdown(QWidget* parent);

        void addSeperator();
        void addItem(const QString& title, std::function<void()> onClick);
        void addItem(const QString& title, bool value,
            std::function<void(bool)> onClick);
        Dropdown addSubmenu(const QString& title);

        QWidget* releaseWidget();

    private:
        Dropdown(QMenu* menu, QWidget* parent);

        QWidget* parent_{};
        std::unique_ptr<QMenu> menu_;
    };
}  // namespace chatterino::ui
