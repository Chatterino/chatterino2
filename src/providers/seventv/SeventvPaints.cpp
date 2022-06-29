#include "SeventvPaints.hpp"

#include "common/NetworkRequest.hpp"
#include "common/Outcome.hpp"
#include "messages/Image.hpp"
#include "providers/seventv/paints/LinearGradientPaint.hpp"
#include "providers/seventv/paints/PaintDropShadow.hpp"
#include "providers/seventv/paints/RadialGradientPaint.hpp"
#include "providers/seventv/paints/UrlPaint.hpp"

#include <QUrl>
#include <QUrlQuery>
#include <vector>

namespace chatterino {
void SeventvPaints::initialize(Settings &settings, Paths &paths)
{
    this->loadSeventvCosmetics();
}

std::optional<std::shared_ptr<Paint>> SeventvPaints::getPaint(
    const QString &userName) const
{
    const auto it = this->paints_.find(userName);
    if (it != this->paints_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

void SeventvPaints::loadSeventvCosmetics()
{
    static QUrl url("https://api.7tv.app/v2/cosmetics");

    static QUrlQuery urlQuery;
    // valid user_identifier values: "object_id", "twitch_id", "login"
    urlQuery.addQueryItem("user_identifier", "login");

    url.setQuery(urlQuery);

    NetworkRequest(url)
        .onSuccess([this](NetworkResult result) -> Outcome {
            auto root = result.parseJson();

            auto jsonPaints = root.value("paints").toArray();
            this->loadSeventvPaints(jsonPaints);

            return Success;
        })
        .execute();
}

void SeventvPaints::loadSeventvPaints(const QJsonArray paints)
{
    for (const auto &jsonPaint : paints)
    {
        std::shared_ptr<Paint> paint;
        const auto paintObject = jsonPaint.toObject();

        const QString name = paintObject.value("name").toString();
        const QStringList userNames =
            this->parsePaintUsers(paintObject.value("users").toArray());

        const auto color = this->parsePaintColor(paintObject.value("color"));
        const bool repeat = paintObject.value("repeat").toBool();
        const float angle = paintObject.value("angle").toDouble();

        const QGradientStops stops =
            this->parsePaintStops(paintObject.value("stops").toArray());

        const auto shadows =
            this->parseDropShadows(paintObject.value("drop_shadows").toArray());

        const QString function = paintObject.value("function").toString();
        if (function == "linear-gradient")
        {
            paint = std::make_shared<LinearGradientPaint>(
                name, color, stops, repeat, angle, shadows);
        }
        else if (function == "radial-gradient")
        {
            const QString shape = paintObject.value("shape").toString();

            paint = std::make_shared<RadialGradientPaint>(name, stops, repeat,
                                                          shadows);
        }
        else if (function == "url")
        {
            const QString url = paintObject.value("image_url").toString();
            const ImagePtr image = Image::fromUrl({url}, 1);
            if (image == nullptr)
            {
                continue;
            }

            paint = std::make_shared<UrlPaint>(name, image, shadows);
        }
        else
        {
            continue;
        }

        for (const auto &userName : userNames)
        {
            this->paints_[userName] = paint;
        }
    }
}

QStringList SeventvPaints::parsePaintUsers(const QJsonArray users) const
{
    QStringList userIds;

    for (const auto &user : users)
    {
        userIds.push_back(user.toString());
    }

    return userIds;
}

std::optional<QColor> SeventvPaints::parsePaintColor(
    const QJsonValue color) const
{
    if (color.isNull())
        return std::nullopt;

    return this->decimalColorToQColor(color.toInt());
}

QGradientStops SeventvPaints::parsePaintStops(const QJsonArray stops) const
{
    QGradientStops parsedStops;
    double lastStop = -1;

    for (const auto &stop : stops)
    {
        const auto stopObject = stop.toObject();

        const auto decimalColor = stopObject.value("color").toInt();
        auto position = stopObject.value("at").toDouble();

        // HACK: qt does not support hard edges in gradients like css does
        // Setting a different color at the same position twice just overwrites
        // the previous color. So we have to shift the second point slightly
        // ahead, simulating an actual hard edge
        if (position == lastStop)
        {
            position += 0.0000001;
        }

        lastStop = position;
        parsedStops.append(
            QGradientStop(position, this->decimalColorToQColor(decimalColor)));
    }

    return parsedStops;
}

std::vector<PaintDropShadow> SeventvPaints::parseDropShadows(
    const QJsonArray dropShadows) const
{
    std::vector<PaintDropShadow> parsedDropShadows;

    for (const auto &shadow : dropShadows)
    {
        const auto shadowObject = shadow.toObject();

        const auto xOffset = shadowObject.value("x_offset").toDouble();
        const auto yOffset = shadowObject.value("y_offset").toDouble();
        const auto radius = shadowObject.value("radius").toDouble();
        const auto decimalColor = shadowObject.value("color").toInt();

        parsedDropShadows.push_back(
            PaintDropShadow(xOffset, yOffset, radius,
                            this->decimalColorToQColor(decimalColor)));
    }

    return parsedDropShadows;
}

QColor SeventvPaints::decimalColorToQColor(const uint32_t color) const
{
    auto red = (color >> 24) & 0xFF;
    auto green = (color >> 16) & 0xFF;
    auto blue = (color >> 8) & 0xFF;
    auto alpha = color & 0xFF;

    return QColor(red, green, blue, alpha);
}

}  // namespace chatterino
