#include "controllers/completion/sources/UserSource.hpp"

#include "controllers/completion/sources/Helpers.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Settings.hpp"
#include "util/Helpers.hpp"

namespace chatterino::completion {

UserSource::UserSource(const Channel *channel,
                       std::unique_ptr<UserStrategy> strategy,
                       ActionCallback callback, bool prependAt)
    : strategy_(std::move(strategy))
    , callback_(std::move(callback))
    , prependAt_(prependAt)
{
    this->initializeFromChannel(channel);
}

void UserSource::update(const QString &query)
{
    this->output_.clear();
    if (this->strategy_)
    {
        this->strategy_->apply(this->items_, this->output_, query);
    }
}

void UserSource::addToListModel(GenericListModel &model, size_t maxCount) const
{
    addVecToListModel(this->output_, model, maxCount,
                      [this](const UserItem &user) {
                          return std::make_unique<InputCompletionItem>(
                              nullptr, user.second, this->callback_);
                      });
}

void UserSource::addToStringList(QStringList &list, size_t maxCount,
                                 bool isFirstWord) const
{
    bool mentionComma = getSettings()->mentionUsersWithComma;
    addVecToStringList(this->output_, list, maxCount,
                       [this, isFirstWord, mentionComma](const UserItem &user) {
                           const auto userMention = formatUserMention(
                               user.second, isFirstWord, mentionComma);
                           QString strTemplate = this->prependAt_
                                                     ? QStringLiteral("@%1 ")
                                                     : QStringLiteral("%1 ");
                           return strTemplate.arg(userMention);
                       });
}

void UserSource::initializeFromChannel(const Channel *channel)
{
    const auto *tc = dynamic_cast<const TwitchChannel *>(channel);
    if (!tc)
    {
        return;
    }

    this->items_ = tc->accessChatters()->all();

    if (getSettings()->alwaysIncludeBroadcasterInUserCompletions)
    {
        auto it = std::find_if(this->items_.begin(), this->items_.end(),
                               [tc](const UserItem &user) {
                                   return user.first == tc->getName();
                               });

        if (it == this->items_.end())
        {
            this->items_.emplace_back(tc->getName(), tc->getDisplayName());
        }
    }
}

const std::vector<UserItem> &UserSource::output() const
{
    return this->output_;
}

}  // namespace chatterino::completion
