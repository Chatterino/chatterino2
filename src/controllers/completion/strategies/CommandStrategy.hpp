// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/completion/sources/CommandSource.hpp"
#include "controllers/completion/strategies/Strategy.hpp"

namespace chatterino::completion {

class CommandStrategy : public Strategy<CommandItem>
{
public:
    CommandStrategy(bool startsWithOnly);

    void apply(const std::vector<CommandItem> &items,
               std::vector<CommandItem> &output,
               const QString &query) const override;

private:
    bool startsWithOnly_;
};

}  // namespace chatterino::completion
