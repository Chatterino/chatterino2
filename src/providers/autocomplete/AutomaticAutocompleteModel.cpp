#include "providers/autocomplete/AutomaticAutocompleteModel.hpp"

#include "providers/autocomplete/AutocompleteCommandsSource.hpp"
#include "providers/autocomplete/AutocompleteCommandStrategies.hpp"
#include "providers/autocomplete/AutocompleteEmoteStrategies.hpp"
#include "providers/autocomplete/AutocompleteUserStrategies.hpp"

namespace chatterino {

AutomaticAutocompleteModel::AutomaticAutocompleteModel(ChannelPtr channel,
                                                       QObject *parent)
    : QStringListModel(parent)
    , channel_(std::move(channel))
{
}

void AutomaticAutocompleteModel::updateResults(const QString &query,
                                               bool isFirstWord)
{
    this->updateSourceFromQuery(query);

    if (this->source_)
    {
        this->source_->update(query);
        this->source_->copyToStringModel(*this, 0, isFirstWord);
    }
}

void AutomaticAutocompleteModel::updateSourceFromQuery(const QString &query)
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
                this->channel_, nullptr,
                std::make_unique<ClassicAutocompleteEmoteStrategy>());
            break;
        case SourceKind::User:
            this->source_ = std::make_unique<AutocompleteUsersSource>(
                this->channel_, nullptr,
                std::make_unique<ClassicAutocompleteUserStrategy>());
            break;
        case SourceKind::Command:
            this->source_ = std::make_unique<AutocompleteCommandsSource>(
                nullptr, std::make_unique<AutocompleteCommandStrategy>(true));
            break;
    }
}

boost::optional<AutomaticAutocompleteModel::SourceKind>
    AutomaticAutocompleteModel::deduceSourceKind(const QString &query) const
{
    if (query.length() < 2 ||
        (this->channel_ && !this->channel_->isTwitchChannel()))
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

    return SourceKind::Emote;
}

}  // namespace chatterino
