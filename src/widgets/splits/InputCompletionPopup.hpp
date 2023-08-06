#pragma once

#include "widgets/BasePopup.hpp"
#include "widgets/listview/GenericListModel.hpp"

#include <functional>
#include <memory>
#include <vector>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

namespace detail {

    struct CompletionEmote {
        EmotePtr emote;
        QString displayName;
        QString providerName;
    };

    std::vector<CompletionEmote> buildCompletionEmoteList(const QString &text,
                                                          ChannelPtr channel);

}  // namespace detail

class GenericListView;

class InputCompletionPopup : public BasePopup
{
    using ActionCallback = std::function<void(const QString &)>;

    constexpr static int MAX_ENTRY_COUNT = 200;

public:
    InputCompletionPopup(QWidget *parent = nullptr);

    void updateEmotes(const QString &text, ChannelPtr channel);
    void updateUsers(const QString &text, ChannelPtr channel);

    void setInputAction(ActionCallback callback);

    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void themeChangedEvent() override;

private:
    void initLayout();

    struct {
        GenericListView *listView;
    } ui_;

    GenericListModel model_;
    ActionCallback callback_;
    QTimer redrawTimer_;
};

}  // namespace chatterino
