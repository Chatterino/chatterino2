#pragma once

#include "ab/FlatButton.hpp"

namespace ab
{
    enum class TitleBarButtonType { Close, Minimize, Unmaximize, Maximize };

    class TitleBarButton : public FlatButton
    {
        Q_OBJECT

    public:
        TitleBarButton();

        void setType(TitleBarButtonType type);
        TitleBarButtonType type() const;

    protected:
        void paintEvent(QPaintEvent*) override;

    private:
        TitleBarButtonType type_;
    };
}  // namespace chatterino
