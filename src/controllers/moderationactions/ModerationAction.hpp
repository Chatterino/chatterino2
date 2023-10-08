#pragma once

#include "util/RapidjsonHelpers.hpp"

#include <pajlada/serialize.hpp>
#include <QString>

#include <memory>
#include <optional>

namespace chatterino {

class Image;
using ImagePtr = std::shared_ptr<Image>;

class ModerationAction
{
public:
    ModerationAction(const QString &action);

    bool operator==(const ModerationAction &other) const;

    bool isImage() const;
    const std::optional<ImagePtr> &getImage() const;
    const QString &getLine1() const;
    const QString &getLine2() const;
    const QString &getAction() const;

private:
    mutable std::optional<ImagePtr> image_;
    QString line1_;
    QString line2_;
    QString action_;
    int imageToLoad_{};
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

        return chatterino::ModerationAction(pattern);
    }
};

}  // namespace pajlada
