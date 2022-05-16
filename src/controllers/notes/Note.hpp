#pragma once

#include "controllers/accounts/AccountController.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QString>
#include <pajlada/serialize.hpp>

#include <memory>

namespace chatterino {

class Note
{
public:
    Note(const QString &id, const QString &note)
        : id_(id)
        , note_(note)
    {
    }

    [[nodiscard]] const QString &id() const
    {
        return this->id_;
    }

    [[nodiscard]] const QString &note() const
    {
        return this->note_;
    }

private:
    QString id_;
    QString note_;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::Note> {
    static rapidjson::Value get(const chatterino::Note &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "id", value.id(), a);
        chatterino::rj::set(ret, "note", value.note(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::Note> {
    static chatterino::Note get(const rapidjson::Value &value,
                                    bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::Note(QString(), QString());
        }

        QString _id;
        QString _note;

        chatterino::rj::getSafe(value, "id", _id);
        chatterino::rj::getSafe(value, "note", _note);

        return chatterino::Note(_id, _note);
    }
};

}  // namespace pajlada