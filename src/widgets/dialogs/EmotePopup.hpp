// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "messages/Emote.hpp"
#include "widgets/BasePopup.hpp"

#include <pajlada/signals/signal.hpp>
#include <QLineEdit>

namespace chatterino {

struct Link;
class ChannelView;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
class Notebook;
class TwitchChannel;

class EmotePopup : public BasePopup
{
public:
    EmotePopup(QWidget *parent = nullptr);

    void loadChannel(ChannelPtr channel);

    void closeEvent(QCloseEvent *event) override;

    pajlada::Signals::Signal<Link> linkClicked;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
    void themeChangedEvent() override;

private:
    ChannelView *globalEmotesView_{};
    ChannelView *channelEmotesView_{};
    ChannelView *subEmotesView_{};
    ChannelView *viewEmojis_{};
    ChannelView *favEmotesView_{};
    /**
     * @brief Visible only when the user has specified a search query into the `search_` input.
     * Otherwise the `notebook_` and all other views are visible.
     */
    ChannelView *searchView_{};

    ChannelPtr channel_;
    TwitchChannel *twitchChannel_{};

    QLineEdit *search_;
    Notebook *notebook_;

    std::vector<EmotePtr> favEmotes_;

    void filterTwitchEmotes(std::shared_ptr<Channel> searchChannel,
                            const QString &searchText);
    void filterEmotes(const QString &text);
    EmotePtr findEmote(const EmoteName &name);
    void addShortcuts() override;
    bool eventFilter(QObject *object, QEvent *event) override;

    void reloadEmotes();

    void addFavouriteEmote(const EmoteName &name);
    void removeFavouriteEmote(const EmoteName &name);
    void updateFavouriteEmotes();

    void saveBounds() const;
};

}  // namespace chatterino
