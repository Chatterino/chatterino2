#include "controllers/completion/TabCompletionModel.hpp"

#include "common/Channel.hpp"
#include "controllers/completion/sources/CommandSource.hpp"
#include "controllers/completion/sources/EmoteSource.hpp"
#include "controllers/completion/sources/UnifiedSource.hpp"
#include "controllers/completion/sources/UserSource.hpp"
#include "controllers/completion/strategies/ClassicEmoteStrategy.hpp"
#include "controllers/completion/strategies/ClassicUserStrategy.hpp"
#include "controllers/completion/strategies/CommandStrategy.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

TabCompletionModel::TabCompletionModel(Channel &channel, QObject *parent)
    : QStringListModel(parent)
    , channel_(channel)
{
}

void TabCompletionModel::updateResults(const QString &query, bool isFirstWord)
{
    this->updateSourceFromQuery(query);

    if (this->source_)
    {
        this->source_->update(query);

        // Copy results to this model
        QStringList results;
        this->source_->addToStringList(results, 0, isFirstWord);
        this->setStringList(results);
    }
}

void TabCompletionModel::updateSourceFromQuery(const QString &query)
{
    auto deducedKind = this->deduceSourceKind(query);
    if (!deducedKind)
    {
        // unable to determine what kind of completion is occurring
        this->sourceKind_ = std::nullopt;
        this->source_ = nullptr;
        return;
    }

    if (this->sourceKind_ == *deducedKind)
    {
        // Source already properly configured
        return;
    }

    this->sourceKind_ = *deducedKind;
    this->source_ = this->buildSource(*deducedKind);
}

std::optional<TabCompletionModel::SourceKind>
    TabCompletionModel::deduceSourceKind(const QString &query) const
{
    if (query.length() < 2 || !this->channel_.isTwitchChannel())
    {
        return std::nullopt;
    }

    // Check for cases where we can definitively say what kind of completion is taking place.

    if (query.startsWith('@'))
    {
        return SourceKind::User;
    }
    else if (query.startsWith(':'))
    {
        return SourceKind::Emote;
    }
    else if (query.startsWith('/') || query.startsWith('.'))
    {
        return SourceKind::Command;
    }

    // At this point, we note that emotes can be completed without using a :
    // Therefore, we must also consider that the user could be completing an emote
    // OR a mention depending on their completion settings.

    if (getSettings()->userCompletionOnlyWithAt)
    {
        return SourceKind::Emote;
    }

    // Either is possible, use unified source
    return SourceKind::EmoteAndUser;
}

std::unique_ptr<completion::Source> TabCompletionModel::buildSource(
    SourceKind kind) const
{
    switch (kind)
    {
        case SourceKind::Emote:
            return std::make_unique<completion::EmoteSource>(
                &this->channel_,
                std::make_unique<completion::ClassicTabEmoteStrategy>());
        case SourceKind::User:
            return std::make_unique<completion::UserSource>(
                &this->channel_,
                std::make_unique<completion::ClassicUserStrategy>());
        case SourceKind::Command:
            return std::make_unique<completion::CommandSource>(
                std::make_unique<completion::CommandStrategy>(true));
        case SourceKind::EmoteAndUser:
            return std::make_unique<completion::UnifiedSource>(
                &this->channel_,
                std::make_unique<completion::ClassicTabEmoteStrategy>(),
                std::make_unique<completion::ClassicUserStrategy>());
        default:
            return nullptr;
    }
}

}  // namespace chatterino
