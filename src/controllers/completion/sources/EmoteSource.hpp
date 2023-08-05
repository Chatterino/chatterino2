#pragma once

#include "common/Channel.hpp"
#include "controllers/completion/sources/Source.hpp"
#include "controllers/completion/strategies/Strategy.hpp"
#include "messages/Emote.hpp"

#include <QString>

#include <functional>
#include <memory>
#include <vector>

namespace chatterino::completion {

struct EmoteItem {
    /// Emote image to show in input popup
    EmotePtr emote{};
    /// Name to check completion queries against
    QString searchName{};
    /// Name to insert into split input upon tab completing
    QString tabCompletionName{};
    /// Display name within input popup
    QString displayName{};
    /// Emote provider name for input popup
    QString providerName{};
    /// Whether emote is emoji
    bool isEmoji{};
};

class EmoteSource : public Source
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using EmoteStrategy = Strategy<EmoteItem>;

    /// @brief Initializes a source for EmoteItems from the given channel
    /// @param channel Channel to initialize emotes from
    /// @param strategy Strategy to apply
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    EmoteSource(const Channel *channel, std::unique_ptr<EmoteStrategy> strategy,
                ActionCallback callback = nullptr);

    void update(const QString &query) override;
    void addToListModel(GenericListModel &model,
                        size_t maxCount = 0) const override;
    void addToStringList(QStringList &list, size_t maxCount = 0,
                         bool isFirstWord = false) const override;

    const std::vector<EmoteItem> &output() const;

private:
    void initializeFromChannel(const Channel *channel);

    std::unique_ptr<EmoteStrategy> strategy_;
    ActionCallback callback_;

    std::vector<EmoteItem> items_{};
    std::vector<EmoteItem> output_{};
};

}  // namespace chatterino::completion
