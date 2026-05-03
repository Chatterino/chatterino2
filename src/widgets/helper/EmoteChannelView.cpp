// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "widgets/helper/EmoteChannelView.hpp"

#include "messages/layouts/MessageLayoutElement.hpp"
#include "singletons/Settings.hpp"

namespace {

using namespace chatterino;

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

bool isFavouriteEmoteOrEmoji(const MessageElement *element)
{
    const auto &identifier = element->getLink().value;

    if (isEmojiIdentifier(identifier))
    {
        const auto &emojiNames =
            Settings::instance().favouriteEmojis.getValue();
        auto shortCode = emojiIdentifierToShortCode(identifier);

        auto it =
            std::ranges::find_if(emojiNames, [&shortCode](const auto &sc) {
                return shortCode == sc;
            });
        return it != emojiNames.end();
    }

    const auto &emoteNames = Settings::instance().favouriteEmotes.getValue();
    auto it =
        std::ranges::find_if(emoteNames, [identifier](const auto &emoteName) {
            return emoteName == identifier;
        });
    return it != emoteNames.end();
}

}  // namespace

namespace chatterino {

EmoteChannelView::EmoteChannelView(QWidget *parent)
    : ChannelView(parent)
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

    if (const auto *emoteElement =
            dynamic_cast<const EmoteElement *>(&hoveredElement->getCreator()))
    {
        this->addFavouriteContextMenuItems(menu, emoteElement);
        addImageContextMenuItems(menu, hoveredElement);
    }
    else if (const auto *textElement = dynamic_cast<const TextElement *>(
                 &hoveredElement->getCreator()))
    {
        this->addFavouriteContextMenuItems(menu, textElement);
    }

    menu->popup(QCursor::pos());
    menu->raise();
}

void EmoteChannelView::addFavouriteContextMenuItems(
    QMenu *menu, const MessageElement *element)
{
    auto *favouriteAction = menu->addAction("Favourite");
    favouriteAction->setCheckable(true);
    favouriteAction->setChecked(isFavouriteEmoteOrEmoji(element));
    menu->addSeparator();

    QObject::connect(
        favouriteAction, &QAction::triggered, [this, element](bool checked) {
            const auto &identifier = element->getLink().value;
            this->favouriteStateChanged.invoke(identifier, checked);
        });
}

}  // namespace chatterino
