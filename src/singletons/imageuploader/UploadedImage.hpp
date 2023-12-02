#pragma once
#include "util/RapidjsonHelpers.hpp"

#include <QString>
#include <Qt>

#include <cstdint>

namespace chatterino {
struct UploadedImage {
    QString channelName;
    QString deletionLink;
    QString imageLink;
    QString localPath;
    int64_t timestamp{};
};
}  // namespace chatterino

namespace pajlada {
template <>
struct Serialize<chatterino::UploadedImage> {
    static rapidjson::Value get(const chatterino::UploadedImage &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "channelName", value.channelName, a);
        chatterino::rj::set(ret, "imageLink", value.imageLink, a);
        chatterino::rj::set(ret, "timestamp", value.timestamp, a);
        chatterino::rj::set(ret, "localPath", value.localPath, a);
        chatterino::rj::set(ret, "deletionLink", value.deletionLink, a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::UploadedImage> {
    static chatterino::UploadedImage get(const rapidjson::Value &value,
                                         bool *error = nullptr)
    {
        chatterino::UploadedImage img;

        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }

        if (value["localPath"].IsNull())
        {
            img.localPath = QString();
        }
        else if (!chatterino::rj::getSafe(value, "localPath", img.localPath))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }
        if (!chatterino::rj::getSafe(value, "imageLink", img.imageLink))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }
        if (value["deletionLink"].IsNull())
        {
            img.deletionLink = QString();
        }
        else if (!chatterino::rj::getSafe(value, "deletionLink",
                                          img.deletionLink))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }
        if (!chatterino::rj::getSafe(value, "channelName", img.channelName))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }
        if (!chatterino::rj::getSafe(value, "timestamp", img.timestamp))
        {
            PAJLADA_REPORT_ERROR(error);
            return img;
        }

        return img;
    }
};
}  // namespace pajlada
