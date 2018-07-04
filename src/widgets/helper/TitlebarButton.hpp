#pragma once

#include "widgets/helper/RippleEffectButton.hpp"

namespace chatterino {

class TitleBarButton : public RippleEffectButton
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
    void setButtonStyle(Style style);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    Style style;
};

}  // namespace chatterino
