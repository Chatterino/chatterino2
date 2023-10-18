#pragma once

#include "controllers/completion/CompletionModel.hpp"
#include "controllers/completion/sources/Source.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/listview/GenericListView.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace chatterino {

class Channel;
using ChannelPtr = std::shared_ptr<Channel>;

class GenericListView;

class InputCompletionPopup : public BasePopup
{
    using ActionCallback = std::function<void(const QString &)>;

    constexpr static size_t MAX_ENTRY_COUNT = 200;

public:
    InputCompletionPopup(QWidget *parent = nullptr);

    void updateCompletion(const QString &text, CompletionKind kind,
                          ChannelPtr channel);

    void setInputAction(ActionCallback callback);

    bool eventFilter(QObject *watched, QEvent *event) override;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

    void themeChangedEvent() override;

private:
    void initLayout();
    void beginCompletion(CompletionKind kind, ChannelPtr channel);
    void endCompletion();

    std::unique_ptr<completion::Source> getSource() const;

    struct {
        GenericListView *listView;
    } ui_{};

    CompletionModel model_;
    ActionCallback callback_;
    QTimer redrawTimer_;

    std::optional<CompletionKind> currentKind_{};
    ChannelPtr currentChannel_{};
};

}  // namespace chatterino
