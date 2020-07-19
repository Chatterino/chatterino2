#pragma once

namespace chatterino {

class AbstractSwitcherItem : public QListWidgetItem
{
public:
    AbstractSwitcherItem(const QString &text);
    AbstractSwitcherItem(const QIcon &icon, const QString &text);

    virtual void action() = 0;
};

}  // namespace chatterino
