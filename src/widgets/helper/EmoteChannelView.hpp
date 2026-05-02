// SPDX-FileCopyrightText: 2016 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "providers/emoji/Emojis.hpp"
#include "widgets/helper/ChannelView.hpp"

#include <QObject>

namespace chatterino {

class EmoteChannelView : public ChannelView
{
    Q_OBJECT

public:
    EmoteChannelView(const std::vector<EmotePtr> &favEmotes,
                     const std::unordered_map<QString, EmojiPtr> &favEmojis,
                     QWidget *parent);

    pajlada::Signals::Signal<const QString &, bool> favouriteStateChanged;

private:
    void addContextMenuItems(const MessageLayoutElement *hoveredElement,
                             MessageLayoutPtr layout, QMouseEvent *event) override;

    bool isFavouriteEmoteOrEmoji(const EmoteElement *element);

    const std::vector<EmotePtr> &favEmotes_;
    const std::unordered_map<QString, EmojiPtr> &favEmojis_;
};

}  // namespace chatterino
