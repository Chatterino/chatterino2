#pragma once

#include "common/Channel.hpp"
#include "providers/autocomplete/AutocompleteSource.hpp"

#include <QString>

#include <functional>
#include <memory>
#include <utility>

namespace chatterino {

struct CompleteCommand {
    QString name;
    QChar prefix;
};

class AutocompleteCommandsSource
    : public AutocompleteGenericSource<CompleteCommand>
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using AutocompleteCommandStrategy = AutocompleteStrategy<CompleteCommand>;

    AutocompleteCommandsSource(
        ActionCallback callback,
        std::unique_ptr<AutocompleteCommandStrategy> strategy);

protected:
    std::unique_ptr<GenericListItem> mapListItem(
        const CompleteCommand &command) const override;

    QString mapTabStringItem(const CompleteCommand &command,
                             bool isFirstWord) const override;

private:
    void initializeItems();

    ActionCallback callback_;
};

}  // namespace chatterino
