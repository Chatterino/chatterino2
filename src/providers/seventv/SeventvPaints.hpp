#pragma once

#include "common/Singleton.hpp"
#include "providers/seventv/paints/Paint.hpp"
#include "providers/seventv/paints/PaintDropShadow.hpp"

#include <optional>

#include <QJsonArray>
#include <QString>

namespace chatterino {

class SeventvPaints : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    std::optional<std::shared_ptr<Paint>> getPaint(
        const QString &userName) const;

private:
    void loadSeventvCosmetics();
    void loadSeventvPaints(const QJsonArray paints);

    QStringList parsePaintUsers(const QJsonArray users) const;
    std::optional<QColor> parsePaintColor(const QJsonValue color) const;
    QGradientStops parsePaintStops(const QJsonArray stops) const;
    std::vector<PaintDropShadow> parseDropShadows(
        const QJsonArray dropShadows) const;

    QColor decimalColorToQColor(const uint32_t color) const;

    std::map<QString, std::shared_ptr<Paint>> paints_;
};

}  // namespace chatterino
