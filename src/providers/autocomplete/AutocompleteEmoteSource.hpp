#pragma once

#include "common/Channel.hpp"
#include "messages/Emote.hpp"
#include "providers/autocomplete/AutocompleteSource.hpp"

#include <QString>

#include <functional>
#include <memory>

namespace chatterino {

struct CompletionEmote {
    // emote image to show in input popup
    EmotePtr emote;
    // name to check completion queries against
    QString searchName;
    // name to insert into split input upon tab completing
    QString tabCompletionName;
    // display name within input popup
    QString displayName;
    // emote provider name for input popup
    QString providerName;
};

class AutocompleteEmoteSource
    : public AutocompleteGenericSource<CompletionEmote>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteEmoteStrategy = AutocompleteStrategy<CompletionEmote>;

    AutocompleteEmoteSource(
        const Channel *channel, ActionCallback callback,
        std::unique_ptr<AutocompleteEmoteStrategy> strategy);

protected:
    std::unique_ptr<GenericListItem> mapListItem(
        const CompletionEmote &emote) const override;

    QString mapTabStringItem(const CompletionEmote &emote,
                             bool isFirstWord) const override;

private:
    void initializeItems(const Channel *channel);

    ActionCallback callback_;
};

}  // namespace chatterino
