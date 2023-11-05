#pragma once

#include "common/Channel.hpp"
#include "controllers/completion/sources/Source.hpp"
#include "controllers/completion/strategies/Strategy.hpp"

#include <QString>

#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace chatterino::completion {

using UserItem = std::pair<QString, QString>;

class UserSource : public Source
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using UserStrategy = Strategy<UserItem>;

    /// @brief Initializes a source for UserItems from the given channel.
    /// @param channel Channel to initialize users from. Must be a TwitchChannel
    /// or completion is a no-op.
    /// @param strategy Strategy to apply
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    /// @param prependAt Whether to prepend @ to string completion suggestions.
    UserSource(const Channel *channel, std::unique_ptr<UserStrategy> strategy,
               ActionCallback callback = nullptr, bool prependAt = true);

    void update(const QString &query) override;
    void addToListModel(GenericListModel &model,
                        size_t maxCount = 0) const override;
    void addToStringList(QStringList &list, size_t maxCount = 0,
                         bool isFirstWord = false) const override;

    const std::vector<UserItem> &output() const;

private:
    void initializeFromChannel(const Channel *channel);

    std::unique_ptr<UserStrategy> strategy_;
    ActionCallback callback_;
    bool prependAt_;

    std::vector<UserItem> items_{};
    std::vector<UserItem> output_{};
};

}  // namespace chatterino::completion
