// SPDX-FileCopyrightText: 2016 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/EmoteChannelView.hpp"

#include "messages/Emote.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "providers/emoji/Emojis.hpp"

namespace {

bool isEmojiIdentifier(const QString &identifier)
{
    return identifier.startsWith(':') && identifier.endsWith(':') &&
           identifier.length() >= 3;
}

QString emojiIdentifierToShortCode(const QString &identifier)
{
    if (!isEmojiIdentifier(identifier))
    {
        return {};
    }

    return identifier.mid(1, identifier.length() - 2);
}

}  // namespace

namespace chatterino {

EmoteChannelView::EmoteChannelView(
    const std::vector<EmotePtr> &favEmotes,
    const std::unordered_map<QString, EmojiPtr> &favEmojis, QWidget *parent)
    : ChannelView(parent)
    , favEmotes_(favEmotes)
    , favEmojis_(favEmojis)
{
}

void EmoteChannelView::addContextMenuItems(
    const MessageLayoutElement *hoveredElement, MessageLayoutPtr, QMouseEvent *)
{
    if (hoveredElement == nullptr)
    {
        return;
    }

    auto *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    const auto *emoteElement =
        dynamic_cast<const EmoteElement *>(&hoveredElement->getCreator());

    if (emoteElement)
    {
        auto *favouriteAction = menu->addAction("Favourite");
        favouriteAction->setCheckable(true);
        favouriteAction->setChecked(isFavouriteEmoteOrEmoji(emoteElement));

        QObject::connect(
            favouriteAction, &QAction::triggered,
            [this, emoteElement](bool checked) {
                const auto &identifier = emoteElement->getLink().value;
                this->favouriteStateChanged.invoke(identifier, checked);
            });

        menu->addSeparator();
    }

    addImageContextMenuItems(menu, hoveredElement);

    menu->popup(QCursor::pos());
    menu->raise();
}

bool EmoteChannelView::isFavouriteEmoteOrEmoji(const EmoteElement *element)
{
    const auto &identifier = element->getLink().value;

    if (isEmojiIdentifier(identifier))
    {
        return this->favEmojis_.contains(
            emojiIdentifierToShortCode(identifier));
    }

    auto it =
        std::ranges::find_if(this->favEmotes_, [identifier](const auto &emote) {
            return emote->name.string == identifier;
        });
    return it != this->favEmotes_.end();
}

}  // namespace chatterino
