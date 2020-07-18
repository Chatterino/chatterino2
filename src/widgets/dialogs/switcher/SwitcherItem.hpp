#pragma once

namespace chatterino {

class SwitcherItem : public QListWidgetItem
{
public:
    SwitcherItem(const QString &text);
    SwitcherItem(const QIcon &icon, const QString &text);

    virtual void action() = 0;
};

}  // namespace chatterino
