#pragma once

#include "common/Channel.hpp"
#include "controllers/completion/sources/EmoteSource.hpp"
#include "controllers/completion/sources/Source.hpp"
#include "controllers/completion/sources/UserSource.hpp"

#include <functional>
#include <memory>

namespace chatterino::completion {

class UnifiedSource : public Source
{
public:
    using ActionCallback = std::function<void(const QString &)>;

    /// @brief Initializes a unified completion source for the given channel.
    /// Resolves both emotes and usernames for autocompletion.
    /// @param channel Channel to initialize emotes and users from. Must be a
    /// TwitchChannel or completion is a no-op.
    /// @param emoteStrategy Strategy for selecting emotes
    /// @param userStrategy Strategy for selecting users
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    UnifiedSource(const Channel &channel,
                  std::unique_ptr<EmoteSource::EmoteStrategy> emoteStrategy,
                  std::unique_ptr<UserSource::UserStrategy> userStrategy,
                  ActionCallback callback = nullptr);

    void update(const QString &query) override;
    void addToListModel(GenericListModel &model,
                        size_t maxCount = 0) const override;
    void addToStringList(QStringList &list, size_t maxCount = 0,
                         bool isFirstWord = false) const override;

private:
    EmoteSource emoteSource_;
    UserSource usersSource_;
};

}  // namespace chatterino::completion
