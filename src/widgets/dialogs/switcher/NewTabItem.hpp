#pragma once

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

namespace chatterino {

class NewTabItem : public AbstractSwitcherItem
{
public:
    NewTabItem(const QString &channelName);

    virtual void action() override;

private:
    static constexpr const char *TEXT_FORMAT = "Open channel \"%1\" in new tab";
    QString channelName_;
};

}  // namespace chatterino
