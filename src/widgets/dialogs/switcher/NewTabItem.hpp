#pragma once

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

namespace chatterino {

class NewTabItem : public AbstractSwitcherItem
{
public:
    NewTabItem(const QString &text, const QString &channelName);

    virtual void action() override;

private:
    QString channelName_;
};

}  // namespace chatterino
