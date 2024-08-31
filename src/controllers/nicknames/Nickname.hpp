#pragma once

#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QRegularExpression>
#include <QString>

#include <memory>
#include <optional>

namespace chatterino {

class Nickname
{
public:
    Nickname(const QString &name, const QString &replace, const bool isRegex,
             const bool isCaseSensitive)
        : name_(name)
        , replace_(replace)
        , isRegex_(isRegex)
        , isCaseSensitive_(isCaseSensitive)
        , caseSensitivity_(this->isCaseSensitive_ ? Qt::CaseSensitive
                                                  : Qt::CaseInsensitive)
    {
        if (this->isRegex())
        {
            this->regex_ = QRegularExpression(
                name, QRegularExpression::UseUnicodePropertiesOption |
                          (this->isCaseSensitive()
                               ? QRegularExpression::NoPatternOption
                               : QRegularExpression::CaseInsensitiveOption));
        }
    }

    [[nodiscard]] const QString &name() const
    {
        return this->name_;
    }

    [[nodiscard]] const QString &replace() const
    {
        return this->replace_;
    }

    [[nodiscard]] bool isRegex() const
    {
        return this->isRegex_;
    }

    [[nodiscard]] Qt::CaseSensitivity caseSensitivity() const
    {
        return this->caseSensitivity_;
    }

    [[nodiscard]] const bool &isCaseSensitive() const
    {
        return this->isCaseSensitive_;
    }

    [[nodiscard]] std::optional<QString> match(
        const QString &usernameText) const
    {
        if (this->isRegex())
        {
            if (!this->regex_.isValid())
            {
                return std::nullopt;
            }
            if (this->name().isEmpty())
            {
                return std::nullopt;
            }

            auto workingCopy = usernameText;
            workingCopy.replace(this->regex_, this->replace());
            if (workingCopy != usernameText)
            {
                return workingCopy;
            }
        }
        else
        {
            auto res =
                this->name().compare(usernameText, this->caseSensitivity());
            if (res == 0)
            {
                return this->replace();
            }
        }

        return std::nullopt;
    }

private:
    QString name_;
    QString replace_;
    bool isRegex_;
    bool isCaseSensitive_;
    Qt::CaseSensitivity caseSensitivity_;
    QRegularExpression regex_{};
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::Nickname> {
    static rapidjson::Value get(const chatterino::Nickname &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value.name(), a);
        chatterino::rj::set(ret, "replace", value.replace(), a);
        chatterino::rj::set(ret, "isRegex", value.isRegex(), a);
        chatterino::rj::set(ret, "isCaseSensitive", value.isCaseSensitive(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::Nickname> {
    static chatterino::Nickname get(const rapidjson::Value &value,
                                    bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::Nickname(QString(), QString(), false, false);
        }

        QString _name;
        QString _replace;
        bool _isRegex;
        bool _isCaseSensitive;

        chatterino::rj::getSafe(value, "name", _name);
        chatterino::rj::getSafe(value, "replace", _replace);
        chatterino::rj::getSafe(value, "isRegex", _isRegex);
        chatterino::rj::getSafe(value, "isCaseSensitive", _isCaseSensitive);

        return chatterino::Nickname(_name, _replace, _isRegex,
                                    _isCaseSensitive);
    }
};

}  // namespace pajlada
