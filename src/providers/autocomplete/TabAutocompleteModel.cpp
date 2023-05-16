#include "providers/autocomplete/TabAutocompleteModel.hpp"

#include "common/Channel.hpp"
#include "providers/autocomplete/AutocompleteSources.hpp"
#include "providers/autocomplete/AutocompleteStrategies.hpp"

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
        this->source_->copyToStringModel(*this, 0, isFirstWord);
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

    switch (*deducedKind)
    {
        case SourceKind::Emote:
            this->source_ = std::make_unique<AutocompleteEmoteSource>(
                &this->channel_, nullptr,
                std::make_unique<ClassicTabAutocompleteEmoteStrategy>());
            break;
        case SourceKind::User:
            this->source_ = std::make_unique<AutocompleteUsersSource>(
                &this->channel_, nullptr,
                std::make_unique<ClassicAutocompleteUserStrategy>());
            break;
        case SourceKind::Command:
            this->source_ = std::make_unique<AutocompleteCommandsSource>(
                nullptr, std::make_unique<AutocompleteCommandStrategy>(true));
            break;
    }
}

boost::optional<TabAutocompleteModel::SourceKind>
    TabAutocompleteModel::deduceSourceKind(const QString &query) const
{
    if (query.length() < 2 || !this->channel_.isTwitchChannel())
    {
        return boost::none;
    }

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

    // Emotes can be autocompleted without using a :
    return SourceKind::Emote;
}

}  // namespace chatterino
