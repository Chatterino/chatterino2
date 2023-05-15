#pragma once

#include "common/Channel.hpp"
#include "messages/Emote.hpp"
#include "providers/autocomplete/AutocompleteSource.hpp"

#include <QString>

#include <functional>
#include <memory>

namespace chatterino {

struct CompletionEmote {
    QString name;
    EmotePtr emote;
    QString displayName;
    QString providerName;
};

class AutocompleteEmoteSource
    : public AutocompleteGenericSource<CompletionEmote>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteEmoteStrategy = AutocompleteStrategy<CompletionEmote>;

    AutocompleteEmoteSource(
        ChannelPtr channel, ActionCallback callback,
        std::unique_ptr<AutocompleteEmoteStrategy> strategy);

protected:
    std::unique_ptr<GenericListItem> mapListItem(
        const CompletionEmote &emote) const override;

private:
    void initializeItems(ChannelPtr channel);

    ActionCallback callback_;
};

}  // namespace chatterino
