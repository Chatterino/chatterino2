#pragma once

#include <QJsonObject>
#include <QString>

namespace chatterino {
struct EventApiEmoteData {
    QJsonObject json;
    QString name;
    QString baseName;

    EventApiEmoteData(QJsonObject _json, QString _baseName);
};
}  // namespace chatterino
