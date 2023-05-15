#pragma once

#include "providers/autocomplete/AutocompleteModel.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/listview/GenericListView.hpp"

#include <functional>
#include <memory>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class GenericListView;

enum class InputCompletionMode { None, Emote, User };

class InputCompletionPopup : public BasePopup
{
    using ActionCallback = std::function<void(const QString &)>;

    constexpr static size_t MAX_ENTRY_COUNT = 200;

public:
    InputCompletionPopup(QWidget *parent = nullptr);

    void updateCompletion(const QString &text, InputCompletionMode mode,
                          ChannelPtr channel);

    void setInputAction(ActionCallback callback);

    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void initLayout();
    void beginCompletion(InputCompletionMode mode, ChannelPtr channel);
    void endCompletion();

    std::unique_ptr<AutocompleteSource> getSource() const;

    struct {
        GenericListView *listView;
    } ui_;

    AutocompleteModel model_;
    ActionCallback callback_;
    QTimer redrawTimer_;

    InputCompletionMode currentMode_{InputCompletionMode::None};
    ChannelPtr currentChannel_{};
};

}  // namespace chatterino
