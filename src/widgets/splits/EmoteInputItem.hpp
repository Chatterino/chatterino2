#pragma once

#include <functional>
#include "messages/Emote.hpp"
#include "widgets/listview/GenericListItem.hpp"

namespace chatterino {

class EmoteInputItem : public GenericListItem
{
    using ActionCallback = std::function<void(const QString &)>;

public:
    EmoteInputItem(const EmotePtr &emote, const QString &text,
                   ActionCallback action);

    // GenericListItem interface
public:
    virtual void action() override;
    virtual void paint(QPainter *painter, const QRect &rect) const override;
    virtual QSize sizeHint(const QRect &rect) const override;

private:
    EmotePtr emote_;
    QString text_;
    ActionCallback action_;
};

}  // namespace chatterino
