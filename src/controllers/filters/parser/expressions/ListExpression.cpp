#include "ListExpression.hpp"

namespace filterparser {

ListExpression::ListExpression(ExpressionList list)
    : list_(std::move(list)){};

QVariant ListExpression::execute(const ContextMap &context) const
{
    QList<QVariant> results;
    bool allStrings = true;
    for (const auto &exp : this->list_)
    {
        auto res = exp->execute(context);
        if (allStrings && variantIsNot(res.type(), QMetaType::QString))
        {
            allStrings = false;
        }
        results.append(res);
    }

    // if everything is a string return a QStringList for case-insensitive comparison
    if (allStrings)
    {
        QStringList strings;
        strings.reserve(results.size());
        for (const auto &val : results)
        {
            strings << val.toString();
        }
        return strings;
    }
    else
    {
        return results;
    }
}

PossibleType ListExpression::returnType() const
{
    for (const auto &exp : this->list_)
    {
        if (exp->returnType() != QMetaType::QString)
        {
            return QMetaType::QVariantList;
        }
    }

    // Every item evaluates to a string
    return QMetaType::QStringList;
}

bool ListExpression::validateTypes(TypeValidator &validator) const
{
    return true;  // Nothing to do
}

QString ListExpression::debug() const
{
    QStringList debugs;
    for (const auto &exp : this->list_)
    {
        debugs.append(exp->debug());
    }
    return QString("{%1}").arg(debugs.join(", "));
}

QString ListExpression::filterString() const
{
    QStringList strings;
    for (const auto &exp : this->list_)
    {
        strings.append(QString("(%1)").arg(exp->filterString()));
    }
    return QString("{%1}").arg(strings.join(", "));
}

}  // namespace filterparser
