#pragma once

#include "providers/twitch/TwitchChannel.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/Notebook.hpp"

#include <pajlada/signals/signal.hpp>

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
    void loadEmojis();

    virtual void closeEvent(QCloseEvent *event) override;

    pajlada::Signals::Signal<Link> linkClicked;

private:
    ChannelView *globalEmotesView_{};
    ChannelView *channelEmotesView_{};
    ChannelView *subEmotesView_{};
    ChannelView *viewEmojis_{};
    ChannelView *searchView_{};

    ChannelPtr channel_;
    TwitchChannel *twitchChannel_;

    QLineEdit *search_;
    Notebook *notebook_;

    void filterEmotes(const QString &text);
    EmoteMap *filterEmoteMap(const QString &text,
                             std::shared_ptr<const EmoteMap> emotes);
    void addShortcuts() override;
};

}  // namespace chatterino
