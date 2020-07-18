#pragma once

#include "widgets/dialogs/switcher/SwitcherItem.hpp"

namespace chatterino {

class NewTabItem : public SwitcherItem
{
public:
    NewTabItem(const QString &text, const QString &channelName);

    virtual void action() override;

private:
    QString channelName_;
};

}  // namespace chatterino
