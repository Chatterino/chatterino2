#pragma once

#include "widgets/helper/rippleeffectbutton.hpp"

namespace chatterino {
namespace widgets {
class TitleBarButton : public RippleEffectButton
{
public:
    enum Style { Minimize = 1, Maximize = 2, Unmaximize = 4, Close = 8, User = 16, Settings = 32 };

    TitleBarButton();

    Style getButtonStyle() const;
    void setButtonStyle(Style style);

protected:
    virtual void paintEvent(QPaintEvent *) override;

private:
    Style style;
};
}  // namespace widgets
}  // namespace chatterino
