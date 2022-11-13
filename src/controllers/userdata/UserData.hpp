#pragma once

#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QColor>
#include <QString>
#include <boost/optional.hpp>
#include <pajlada/serialize.hpp>

namespace chatterino {

// UserData defines a set of data that is defined for a unique user
// It can contain things like optional replacement color for the user, a unique alias
// or a user note that should be displayed with the user
// Replacement fields should be optional, where none denotes that the field should not be updated for the user
struct UserData {
    boost::optional<QColor> color{boost::none};

    // TODO: User note?
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::UserData> {
    static rapidjson::Value get(const chatterino::UserData &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value obj;
        obj.SetObject();
        if (value.color)
        {
            const auto &color = *value.color;
            chatterino::rj::set(obj, "color",
                                color.name().toUtf8().toStdString(), a);
        }
        return obj;
    }
};

template <>
struct Deserialize<chatterino::UserData> {
    static chatterino::UserData get(const rapidjson::Value &value,
                                    bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::UserData{};
        }

        chatterino::UserData user;

        QString colorString;
        if (chatterino::rj::getSafe(value, "color", colorString))
        {
            QColor color(colorString);
            if (color.isValid())
            {
                user.color = color;
            }
        }

        return user;
    }
};

}  // namespace pajlada
