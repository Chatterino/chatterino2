#pragma once

#include "providers/emoji/Emojis.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/Notebook.hpp"

#include <pajlada/signals/signal.hpp>
#include <QLineEdit>

namespace chatterino {

struct Link;
class ChannelView;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class EmotePopup : public BasePopup
{
public:
    EmotePopup(QWidget *parent = nullptr);

    void loadChannel(ChannelPtr channel);

    virtual void closeEvent(QCloseEvent *event) override;

    pajlada::Signals::Signal<Link> linkClicked;

private:
    ChannelView *globalEmotesView_{};
    ChannelView *channelEmotesView_{};
    ChannelView *subEmotesView_{};
    ChannelView *viewEmojis_{};
    /**
     * @brief Visible only when the user has specified a search query into the `search_` input.
     * Otherwise the `notebook_` and all other views are visible.
     */
    ChannelView *searchView_{};

    ChannelPtr channel_;
    TwitchChannel *twitchChannel_{};

    QLineEdit *search_;
    Notebook *notebook_;

    void loadEmojis(ChannelView &view, EmojiMap &emojiMap);
    void loadEmojis(Channel &channel, EmojiMap &emojiMap, const QString &title);
    void filterTwitchEmotes(std::shared_ptr<Channel> searchChannel,
                            const QString &searchText);
    void filterEmotes(const QString &text);
    EmoteMap *filterEmoteMap(const QString &text,
                             std::shared_ptr<const EmoteMap> emotes);
    void addShortcuts() override;
};

}  // namespace chatterino
