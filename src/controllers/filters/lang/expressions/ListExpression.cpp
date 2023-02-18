#include "ListExpression.hpp"

namespace chatterino::filters {

ListExpression::ListExpression(ExpressionList &&list)
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

    return results;
}

PossibleType ListExpression::synthesizeType(const TypingContext &context) const
{
    std::vector<PossibleType> types;
    types.reserve(this->list_.size());
    bool allStrings = true;
    for (const auto &exp : this->list_)
    {
        auto typ = exp->synthesizeType(context);
        if (!typ)
        {
            return typ;  // Ill-typed
        }

        if (typ != Type::String)
        {
            allStrings = false;
        }

        types.push_back(typ);
    }

    if (types.size() == 2 && types[0] == Type::RegularExpression &&
        types[1] == Type::Int)
    {
        // Specific {RegularExpression, Int} form
        return Type::MatchingSpecifier;
    }

    return allStrings ? Type::StringList : Type::List;
}

QString ListExpression::debug(const TypingContext &context) const
{
    QStringList debugs;
    for (const auto &exp : this->list_)
    {
        debugs.append(QString("%1 : %2")
                          .arg(exp->debug(context))
                          .arg(exp->synthesizeType(context).string()));
    }

    return QString("List(%1)").arg(debugs.join(", "));
}

QString ListExpression::filterString() const
{
    QStringList strings;
    for (const auto &exp : this->list_)
    {
        strings.append(exp->filterString());
    }
    return QString("{%1}").arg(strings.join(", "));
}

}  // namespace chatterino::filters
