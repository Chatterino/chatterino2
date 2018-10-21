#pragma once

#include <QString>
#include <boost/optional.hpp>
#include <pajlada/settings/serialize.hpp>

#include "util/RapidjsonHelpers.hpp"

namespace chatterino {

class Image;
using ImagePtr = std::shared_ptr<Image>;

class ModerationAction
{
public:
    ModerationAction(const QString &action);

    bool operator==(const ModerationAction &other) const;

    bool isImage() const;
    const boost::optional<ImagePtr> &getImage() const;
    const QString &getLine1() const;
    const QString &getLine2() const;
    const QString &getAction() const;

private:
    boost::optional<ImagePtr> image_;
    QString line1_;
    QString line2_;
    QString action_;
};

}  // namespace chatterino

namespace pajlada {
namespace Settings {

    template <>
    struct Serialize<chatterino::ModerationAction> {
        static rapidjson::Value get(const chatterino::ModerationAction &value,
                                    rapidjson::Document::AllocatorType &a)
        {
            rapidjson::Value ret(rapidjson::kObjectType);

            AddMember(ret, "pattern", value.getAction(), a);

            return ret;
        }
    };

    template <>
    struct Deserialize<chatterino::ModerationAction> {
        static chatterino::ModerationAction get(const rapidjson::Value &value)
        {
            if (!value.IsObject())
            {
                return chatterino::ModerationAction(QString());
            }

            QString pattern;

            chatterino::rj::getSafe(value, "pattern", pattern);

            return chatterino::ModerationAction(pattern);
        }
    };

}  // namespace Settings
}  // namespace pajlada
