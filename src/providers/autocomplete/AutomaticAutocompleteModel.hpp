#pragma once

#include "common/Channel.hpp"
#include "providers/autocomplete/AutocompleteSource.hpp"

#include <boost/optional.hpp>
#include <QObject>
#include <QString>
#include <QStringListModel>

namespace chatterino {

class AutomaticAutocompleteModel : public QStringListModel
{
public:
    AutomaticAutocompleteModel(ChannelPtr channel, QObject *parent = nullptr);

    void updateResults(const QString &query, bool isFirstWord = false);

private:
    enum class SourceKind { Emote, User, Command };

    void updateSourceFromQuery(const QString &query);
    boost::optional<SourceKind> deduceSourceKind(const QString &query) const;

    ChannelPtr channel_;
    std::unique_ptr<AutocompleteSource> source_{};
    boost::optional<SourceKind> sourceKind_{};
};

}  // namespace chatterino
