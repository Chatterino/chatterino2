#pragma once

#include "common/FlagsEnum.hpp"
#include "widgets/BaseWindow.hpp"

namespace chatterino {

class BasePopup : public BaseWindow
{
public:
    explicit BasePopup(FlagsEnum<BaseWindow::Flags> flags_ = None,
                       QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *e) override;
};

}  // namespace chatterino
