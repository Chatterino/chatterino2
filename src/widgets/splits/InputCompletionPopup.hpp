#pragma once

#include "widgets/BasePopup.hpp"
#include "widgets/listview/GenericListModel.hpp"

#include <functional>
#include <memory>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

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
