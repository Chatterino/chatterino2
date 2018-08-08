#pragma once

#include "widgets/helper/Button.hpp"

namespace chatterino {

class TitleBarButton : public Button
{
public:
    enum Style {
        None = 0,
        Minimize = 1,
        Maximize = 2,
        Unmaximize = 4,
        Close = 8,
        User = 16,
        Settings = 32
    };

    TitleBarButton();

    Style getButtonStyle() const;
    void setButtonStyle(Style style_);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Style style_;
};

}  // namespace chatterino
