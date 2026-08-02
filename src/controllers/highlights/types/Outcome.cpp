// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/Outcome.hpp"

#include "controllers/highlights/Sounds.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <cassert>

namespace chatterino::highlights {

namespace {

constexpr const QStringView NOCOLOR = u"none";

}  // namespace

void Outcome::setSound(const QString &newSound)
{
    this->sound = newSound;

    this->updateSoundURL();
}

std::shared_ptr<QColor> Outcome::getBackgroundColorWithDefault(
    const QColor &defaultColor) const
{
    if (!this->resolvedBackgroundColor)
    {
        return std::make_shared<QColor>(defaultColor);
    }

    return this->resolvedBackgroundColor;
}

void Outcome::serialize(rapidjson::Value &ret,
                        rapidjson::Document::AllocatorType &a) const
{
    rj::setOptionally(ret, "showInMentions", this->showInMentions, a);
    rj::setOptionally(ret, "alert", this->alert, a);

    rj::setOptionally(ret, "sound", this->sound, a);

    if (this->backgroundColor)
    {
        if (this->backgroundColor->isValid())
        {
            // Background color is set
            rj::set(ret, "backgroundColor",
                    this->backgroundColor->name(QColor::HexArgb), a);
        }
        else
        {
            // User has explicitly unset the color
            rj::set(ret, "backgroundColor", NOCOLOR, a);
        }
    }
    else
    {
        // User has not configured this background color, it should resolve to the default color
    }
}

bool Outcome::deserialize(const rapidjson::Value &value)
{
    assert(value.IsObject());

    chatterino::rj::getSafe(value, "showInMentions", this->showInMentions);
    chatterino::rj::getSafe(value, "alert", this->alert);

    chatterino::rj::getSafe(value, "sound", this->sound);

    this->updateSoundURL();

    auto backgroundColorIt = value.FindMember("backgroundColor");
    if (backgroundColorIt != value.MemberEnd())
    {
        QString tmpBackgroundColor;
        chatterino::rj::getSafe(value, "backgroundColor", tmpBackgroundColor);

        assert(!this->backgroundColor);

        // TODO: If this is set to NOCOLOR, do we need to do anything special with it?
        this->setBackgroundColor(tmpBackgroundColor);
    }

    return true;
}

QDebug operator<<(QDebug dbg, const Outcome &v)
{
    const auto &backgroundColorPtr = v.backgroundColor;
    QColor backgroundColor;
    if (backgroundColorPtr)
    {
        backgroundColor = *backgroundColorPtr;
    }
    dbg.nospace() << "Outcome("
                  << "showInMentions:" << v.showInMentions << ','
                  << "alert:" << v.alert << ',' << "sound:" << v.sound << " ("
                  << v.soundURL << "),"
                  << "backgroundColor:" << backgroundColor << ')';

    return dbg;
}

void Outcome::updateSoundURL()
{
    qInfo() << "XXX: Resolving sound URL" << this->sound;
    // TODO: Do we resolve the sound here?
    const auto &billtin = defaultSounds();
    if (this->sound.isNull())
    {
        qInfo() << "XXX: is clear!";
        this->soundURL.clear();
    }
    else
    {
        auto defaultSound = resolveDefaultSound(this->sound);
        if (defaultSound.has_value())
        {
            this->soundURL = defaultSound->resourcePath;
            qInfo() << "XXX: resolved to default sound!" << this->soundURL
                    << defaultSound->resourcePath;

            QUrl xd1(":/sounds/ping2.wav");
            qInfo() << "XXX: xd1" << xd1 << xd1.toString() << xd1.toLocalFile();
            QUrl xd2("qrc:/sounds/ping2.wav");
            qInfo() << "XXX: xd2" << xd2 << xd2.toString(QUrl::RemoveScheme)
                    << xd2.toLocalFile();
        }
        else
        {
            qInfo() << "XXX: did not resolve to default sound";
            this->soundURL = this->sound;
        }
    }
}

}  // namespace chatterino::highlights
