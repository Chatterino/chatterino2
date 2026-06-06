#include "controllers/highlights/types/Outcome.hpp"

#include "util/RapidjsonHelpers.hpp"

#include <cassert>

namespace chatterino::highlights {

namespace {

constexpr const QStringView NOCOLOR = u"none";

}  // namespace

std::shared_ptr<QColor> Outcome::getBackgroundColorWithDefault(
    const QColor &defaultColor) const
{
    if (!this->backgroundColor)
    {
        return std::make_shared<QColor>(defaultColor);
    }

    return this->backgroundColor;
}

void Outcome::serialize(rapidjson::Value &ret,
                        rapidjson::Document::AllocatorType &a) const
{
    rj::setOptionally(ret, "showInMentions", this->showInMentions, a);
    rj::setOptionally(ret, "alert", this->alert, a);
    rj::setOptionally(ret, "playSound", this->playSound, a);
    if (!this->customSoundURL.isEmpty())
    {
        rj::set(ret, "customSoundURL", this->customSoundURL.toString(), a);
    }

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
}

bool Outcome::deserialize(const rapidjson::Value &value)
{
    assert(value.IsObject());

    chatterino::rj::getSafe(value, "showInMentions", this->showInMentions);
    chatterino::rj::getSafe(value, "alert", this->alert);
    chatterino::rj::getSafe(value, "playSound", this->playSound);

    QString tmpCustomSoundURL;
    chatterino::rj::getSafe(value, "customSoundURL", tmpCustomSoundURL);
    if (!tmpCustomSoundURL.isEmpty())
    {
        this->customSoundURL.setUrl(tmpCustomSoundURL);
    }

    auto backgroundColorIt = value.FindMember("backgroundColor");
    if (backgroundColorIt != value.MemberEnd())
    {
        QString tmpBackgroundColor;
        chatterino::rj::getSafe(value, "backgroundColor", tmpBackgroundColor);

        assert(!this->backgroundColor);

        // TODO: If this is set to NOCOLOR, do we need to do anything special with it?
        if (this->backgroundColor)
        {
            *this->backgroundColor = QColor{tmpBackgroundColor};
        }
        else
        {
            this->backgroundColor =
                std::make_shared<QColor>(tmpBackgroundColor);
        }
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
                  << "alert:" << v.alert << ',' << "playSound:" << v.playSound
                  << ',' << "customSoundURL:" << v.customSoundURL << ','
                  << "backgroundColor:" << backgroundColor << ')';

    return dbg;
}

}  // namespace chatterino::highlights
