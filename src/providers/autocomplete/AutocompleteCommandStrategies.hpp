#pragma once

#include "providers/autocomplete/AutocompleteCommandsSource.hpp"
#include "providers/autocomplete/AutocompleteStrategy.hpp"

#include <QString>

namespace chatterino {

class AutocompleteCommandStrategy : public AutocompleteStrategy<CompleteCommand>
{
public:
    AutocompleteCommandStrategy(bool startsWithOnly);

    void apply(const std::vector<CompleteCommand> &items,
               std::vector<CompleteCommand> &output,
               const QString &query) const override;

private:
    bool startsWithOnly_;
};

}  // namespace chatterino
