// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/helper/ChannelView.hpp"

#include <QObject>

namespace chatterino {

class EmoteChannelView : public ChannelView
{
    Q_OBJECT

public:
    EmoteChannelView(QWidget *parent);

    pajlada::Signals::Signal<const QString &, bool> favouriteStateChanged;

private:
    void addContextMenuItems(const MessageLayoutElement *hoveredElement,
                             MessageLayoutPtr layout,
                             QMouseEvent *event) override;
    void addFavouriteContextMenuItems(QMenu *menu,
                                      const MessageElement *element);
};

}  // namespace chatterino
