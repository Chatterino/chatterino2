#pragma once

#include <functional>
#include "widgets/listview/GenericListItem.hpp"

namespace chatterino {

class UsernameInputItem : public GenericListItem
{
    using ActionCallback = std::function<void(const QString &)>;

public:
    UsernameInputItem(const QString &text, ActionCallback action);

    // GenericListItem interface
public:
    virtual void action() override;
    virtual void paint(QPainter *painter, const QRect &rect) const override;
    virtual QSize sizeHint(const QRect &rect) const override;

private:
    QString text_;
    ActionCallback action_;
};

}  // namespace chatterino
