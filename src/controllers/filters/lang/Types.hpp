#pragma once

#include <QString>
#include <QVariant>

#include <memory>
#include <optional>
#include <set>

namespace chatterino {

struct Message;

}

namespace chatterino::filters {

class Expression;

using MessagePtr = std::shared_ptr<const chatterino::Message>;
using ContextMap = QMap<QString, QVariant>;

enum class Type {
    String,
    Int,
    Bool,
    Color,
    RegularExpression,
    List,
    MatchingSpecifier,  // 2-element list in {RegularExpression, Int} form
    Map
};

struct IllTyped {
    const Expression *expr;
    QString message;
};

class PossibleType
{
public:
    // Synthesized type
    PossibleType(Type t);
    // Ill-typed
    PossibleType(IllTyped illTyped);

    QString string() const;

    bool operator==(Type t) const;
    bool operator==(const PossibleType &p) const;
    bool operator!=(Type t) const;
    bool operator!=(const PossibleType &p) const;

    bool well() const;
    operator bool() const;

    const std::optional<IllTyped> &illTypedDescription() const;

private:
    Type type_;
    std::optional<IllTyped> illTyped_;
};

bool isList(PossibleType typ);

inline bool variantIs(const QVariant &a, QMetaType::Type type)
{
    return static_cast<QMetaType::Type>(a.type()) == type;
}

inline bool variantIsNot(const QVariant &a, QMetaType::Type type)
{
    return static_cast<QMetaType::Type>(a.type()) != type;
}

inline bool convertVariantTypes(QVariant &a, QVariant &b, int type)
{
    return a.convert(type) && b.convert(type);
}

inline bool variantTypesMatch(QVariant &a, QVariant &b, QMetaType::Type type)
{
    return variantIs(a, type) && variantIs(b, type);
}

}  // namespace chatterino::filters
