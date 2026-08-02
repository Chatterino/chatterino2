// TODO: COPYRIGHT TEXT

#pragma once

#include <QMetaType>
#include <QString>
#include <QStringView>
#include <QUrl>

#include <map>
#include <optional>

namespace chatterino::highlights {

struct DefaultSound {
    QString id;
    QString displayName;
    QString resourcePath;
};

const std::map<QString, DefaultSound> &defaultSounds();

std::optional<DefaultSound> resolveDefaultSound(QStringView id);

}  // namespace chatterino::highlights

Q_DECLARE_METATYPE(chatterino::highlights::DefaultSound);
