#pragma once

#include <pajlada/signals/signalholder.hpp>
#include "ab/FlatButton.hpp"
#include "ab/Notebook.hpp"

namespace chatterino::ui
{
    enum class TabButtonType { None, Close, Live };

    class TabButton : public ab::FlatButton
    {
        Q_OBJECT

    public:
        void setType(TabButtonType);
        TabButtonType type() const;

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        TabButtonType type_ = TabButtonType::Close;
    };

    class Tab : public ab::BasicTab
    {
        Q_OBJECT

    public:
        Tab();
        Tab(const QString& text);

        void setShowX(bool);
        bool showX() const;
        Q_PROPERTY(bool showX READ showX WRITE setShowX)

    private:
        bool showX_{true};
        TabButton* button_{};

        pajlada::Signals::SignalHolder connections_;
    };
}  // namespace chatterino::ui
