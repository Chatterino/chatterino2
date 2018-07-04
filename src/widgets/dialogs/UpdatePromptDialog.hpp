#pragma once

#include "widgets/BaseWindow.hpp"
#include "widgets/Label.hpp"

namespace chatterino {

class UpdatePromptDialog : public BaseWindow
{
public:
    UpdatePromptDialog();

private:
    struct {
        Label *label;
    } ui_;
};

}  // namespace chatterino
