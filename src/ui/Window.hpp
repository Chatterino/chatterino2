#pragma once

#include <QWidget>

#include "ab/BaseWindow.hpp"
#include "messages/ThemexD.hpp"
#include "ui/UiFwd.hpp"

namespace ab
{
    class Notebook;
}

namespace chatterino::ui
{
    enum class WindowType {
        Main,
        Popup,
    };

    class FlexLayout;

    class Window : public ab::BaseWindow
    {
    public:
        Window(Application& app, WindowType type);

        ThemexD& theme();

    protected:
        void closeEvent(QCloseEvent*) override;
        void showEvent(QShowEvent*) override;
        void dragEnterEvent(QDragEnterEvent*) override;
        void dragLeaveEvent(QDragLeaveEvent*) override;

    private:
        void initLayout();
        void updateLayout();
        void setLeftVisible(bool);
        void setRightVisible(bool);
        void drag();

        // children
        std::unique_ptr<ab::Notebook> left_;
        ab::Notebook* center_{};
        std::unique_ptr<ab::Notebook> right_;
        FlexLayout* layout_{};

        // misc
        ThemexD theme_;
        bool dragging_{};
        WindowType type_;
        Application& app_;
    };
}  // namespace chatterino::ui
