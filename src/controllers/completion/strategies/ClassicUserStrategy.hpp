#pragma once

#include "controllers/completion/sources/UserSource.hpp"
#include "controllers/completion/strategies/Strategy.hpp"

namespace chatterino::completion {

class ClassicUserStrategy : public Strategy<UserItem>
{
    void apply(const std::vector<UserItem> &items,
               std::vector<UserItem> &output,
               const QString &query) const override;
};

}  // namespace chatterino::completion
