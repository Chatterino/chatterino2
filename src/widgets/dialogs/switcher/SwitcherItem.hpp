#pragma once

namespace chatterino {

class SwitcherItem : public QListWidgetItem
{
public:
    SwitcherItem(const QString &title);

    virtual void action() = 0;
};

}  // namespace chatterino
