#include "providers/autocomplete/TabAutocompleteModel.hpp"

#include "common/Channel.hpp"
#include "providers/autocomplete/AutocompleteSources.hpp"
#include "providers/autocomplete/AutocompleteStrategies.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

TabAutocompleteModel::TabAutocompleteModel(Channel &channel, QObject *parent)
    : QStringListModel(parent)
    , channel_(channel)
{
}

void TabAutocompleteModel::updateResults(const QString &query, bool isFirstWord)
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

void TabAutocompleteModel::updateSourceFromQuery(const QString &query)
{
    auto deducedKind = this->deduceSourceKind(query);
    if (!deducedKind)
    {
        // unable to determine what kind of autocomplete is occurring
        this->sourceKind_ = boost::none;
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

boost::optional<TabAutocompleteModel::SourceKind>
    TabAutocompleteModel::deduceSourceKind(const QString &query) const
{
    if (query.length() < 2 || !this->channel_.isTwitchChannel())
    {
        return boost::none;
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

    // At this point, we note that emotes can be autocompleted without using a :
    // Therefore, we must also consider that the user could be completing an emote
    // OR a mention depending on their completion settings.

    if (getSettings()->userCompletionOnlyWithAt)
    {
        return SourceKind::Emote;
    }

    // Either is possible, use unified source
    return SourceKind::EmoteAndUser;
}

std::unique_ptr<AutocompleteSource> TabAutocompleteModel::buildSource(
    SourceKind kind) const
{
    switch (kind)
    {
        case SourceKind::Emote:
            return std::make_unique<AutocompleteEmoteSource>(
                this->channel_,
                std::make_unique<ClassicTabAutocompleteEmoteStrategy>());
        case SourceKind::User:
            return std::make_unique<AutocompleteUsersSource>(
                this->channel_,
                std::make_unique<ClassicAutocompleteUserStrategy>());
        case SourceKind::Command:
            return std::make_unique<AutocompleteCommandsSource>(
                std::make_unique<AutocompleteCommandStrategy>(true));
        case SourceKind::EmoteAndUser:
            return std::make_unique<AutocompleteUnifiedSource>(
                this->channel_,
                std::make_unique<ClassicTabAutocompleteEmoteStrategy>(),
                std::make_unique<ClassicAutocompleteUserStrategy>());
        default:
            return nullptr;
    }
}

}  // namespace chatterino
