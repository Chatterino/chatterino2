#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QString>
#include <QUrl>

#include <memory>
#include <optional>

namespace chatterino {

class Image;
using ImagePtr = std::shared_ptr<Image>;

class ModerationAction
{
public:
    /**
     * Type of the action, parsed from the input `action`
     */
    enum class Type {
        /**
         * /ban <user>
         */
        Ban,

        /**
         * /delete <msg-id>
         */
        Delete,

        /**
         * /timeout <user> <duration>
         */
        Timeout,

        /**
         * Anything not matching the action types above
         */
        Custom,
    };

    ModerationAction(const QString &action, const QUrl &iconPath = {});

    bool operator==(const ModerationAction &other) const;

    bool isImage() const;
    const std::optional<ImagePtr> &getImage() const;
    const QString &getLine1() const;
    const QString &getLine2() const;
    const QString &getAction() const;
    const QUrl &iconPath() const;
    Type getType() const;

private:
    mutable std::optional<ImagePtr> image_;
    QString line1_;
    QString line2_;
    QString action_;

    Type type_{};

    QUrl iconPath_;
};

}  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::ModerationAction> {
    static rapidjson::Value get(const chatterino::ModerationAction &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "pattern", value.getAction(), a);
        chatterino::rj::set(ret, "icon", value.iconPath().toString(), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::ModerationAction> {
    static chatterino::ModerationAction get(const rapidjson::Value &value,
                                            bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return chatterino::ModerationAction(QString());
        }

        QString pattern;
        chatterino::rj::getSafe(value, "pattern", pattern);

        QString icon;
        chatterino::rj::getSafe(value, "icon", icon);

        return chatterino::ModerationAction(pattern, QUrl(icon));
    }
};

}  // namespace pajlada
