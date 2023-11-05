#pragma once

#include "controllers/completion/sources/EmoteSource.hpp"
#include "controllers/completion/strategies/Strategy.hpp"

namespace chatterino::completion {

class ClassicEmoteStrategy : public Strategy<EmoteItem>
{
    void apply(const std::vector<EmoteItem> &items,
               std::vector<EmoteItem> &output,
               const QString &query) const override;
};

class ClassicTabEmoteStrategy : public Strategy<EmoteItem>
{
    void apply(const std::vector<EmoteItem> &items,
               std::vector<EmoteItem> &output,
               const QString &query) const override;
};

}  // namespace chatterino::completion
