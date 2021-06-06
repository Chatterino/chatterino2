#pragma once

#include <functional>
#include "common/Channel.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/listview/GenericListModel.hpp"

namespace chatterino {

class GenericListView;

class UsernameInputPopup : public BasePopup
{
    using ActionCallback = std::function<void(const QString &)>;

    constexpr static int maxUsernameCount = 200;

public:
    UsernameInputPopup(QWidget *parent = nullptr);

    void updateUsers(const QString &text, ChannelPtr channel);
    virtual bool eventFilter(QObject *, QEvent *event) override;

    void setInputAction(ActionCallback callback);

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
