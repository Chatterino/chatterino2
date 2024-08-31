#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QString>

namespace chatterino {

struct Command {
    QString name;
    QString func;
    bool showInMsgContextMenu{};

    Command() = default;
    explicit Command(const QString &text);
    Command(const QString &name, const QString &func,
            bool showInMsgContextMenu = false);

    QString toString() const;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::Command> {
    static rapidjson::Value get(const chatterino::Command &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value.name, a);
        chatterino::rj::set(ret, "func", value.func, a);
        chatterino::rj::set(ret, "showInMsgContextMenu",
                            value.showInMsgContextMenu, a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::Command> {
    static chatterino::Command get(const rapidjson::Value &value,
                                   bool *error = nullptr)
    {
        chatterino::Command command;

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return command;
        }

        if (!chatterino::rj::getSafe(value, "name", command.name))
        {
            PAJLADA_REPORT_ERROR(error);
            return command;
        }
        if (!chatterino::rj::getSafe(value, "func", command.func))
        {
            PAJLADA_REPORT_ERROR(error);
            return command;
        }
        if (!chatterino::rj::getSafe(value, "showInMsgContextMenu",
                                     command.showInMsgContextMenu))
        {
            command.showInMsgContextMenu = false;

            PAJLADA_REPORT_ERROR(error);

            return command;
        }

        return command;
    }
};

}  // namespace pajlada
