#pragma once

#include <functional>
#include "common/Channel.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/listview/GenericListModel.hpp"

namespace chatterino {

class GenericListView;

class EmoteInputPopup : public BasePopup
{
    using ActionCallback = std::function<void(const QString &)>;

    constexpr static int maxLineCount = 10;

public:
    EmoteInputPopup(QWidget *parent = nullptr);

    void updateEmotes(const QString &text, ChannelPtr channel);
    virtual bool eventFilter(QObject *, QEvent *event) override;

    void setInputAction(ActionCallback callback);

private:
    void initLayout();

    struct {
        GenericListView *listView;
    } ui_;

    GenericListModel model_;
    ActionCallback callback_;
    //    pajlada::Signals::SignalHolder connections_;
};

}  // namespace chatterino
