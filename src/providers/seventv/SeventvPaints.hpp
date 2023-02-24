#pragma once

#include "common/Aliases.hpp"
#include "common/NetworkRequest.hpp"
#include "common/Singleton.hpp"
#include "providers/seventv/paints/Paint.hpp"
#include "providers/seventv/paints/PaintDropShadow.hpp"

#include <QJsonArray>
#include <QString>

#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace chatterino {

class SeventvPaints : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;

    void addPaint(const QJsonObject &paintJson);
    void assignPaintToUser(const QString &paintID, const UserName &userName);
    void clearPaintFromUser(const QString &paintID, const UserName &userName);

    std::optional<std::shared_ptr<Paint>> getPaint(
        const QString &userName) const;

private:
    void loadSeventvPaints();

    // Mutex for both `paintMap_` and `knownPaints_`
    mutable std::shared_mutex mutex_;

    // user-name => paint
    std::unordered_map<QString, std::shared_ptr<Paint>> paintMap_;
    // paint-id => paint
    std::unordered_map<QString, std::shared_ptr<Paint>> knownPaints_;
};

}  // namespace chatterino
