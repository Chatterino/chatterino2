// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/filters/lang/Filter.hpp"

#include "controllers/filters/lang/FilterParser.hpp"

namespace chatterino::filters {

FilterResult Filter::fromString(const QString &str)
{
    FilterParser parser(str);

    if (parser.valid())
    {
        auto exp = parser.release();
        auto typ = parser.returnType();
        return Filter(std::move(exp), typ);
    }

    return FilterError{parser.errors().join("\n")};
}

Filter::Filter(ExpressionPtr expression, Type returnType)
    : expression_(std::move(expression))
    , returnType_(returnType)
{
}

Type Filter::returnType() const
{
    return this->returnType_;
}

QVariant Filter::execute(RunContext context) const
{
    return this->expression_->execute(context);
}

QString Filter::filterString() const
{
    return this->expression_->filterString();
}

QString Filter::debugString() const
{
    return this->expression_->debug();
}

}  // namespace chatterino::filters
