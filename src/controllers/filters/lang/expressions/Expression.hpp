// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/filters/lang/Tokenizer.hpp"
#include "controllers/filters/lang/Types.hpp"

#include <QString>
#include <QVariant>

#include <memory>
#include <vector>

namespace chatterino {
struct Message;
class Channel;
}  // namespace chatterino

namespace chatterino::filters {

struct RunContext {
    const Message &message;
    Channel *channel;
};

class Expression
{
public:
    virtual ~Expression() = default;

    virtual QVariant execute(const RunContext &context) = 0;
    virtual PossibleType synthesizeType() const = 0;
    virtual QString debug() const = 0;
    virtual QString filterString() const = 0;
};

using ExpressionPtr = std::unique_ptr<Expression>;
using ExpressionList = std::vector<std::unique_ptr<Expression>>;

}  // namespace chatterino::filters
