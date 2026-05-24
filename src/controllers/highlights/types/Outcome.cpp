#include "controllers/highlights/types/Outcome.hpp"

#include "util/RapidjsonHelpers.hpp"

namespace chatterino::highlights {

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

    if (this->backgroundColor->isValid())
    {
        rj::set(ret, "backgroundColor",
                this->backgroundColor->name(QColor::HexArgb), a);
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

    QString tmpBackgroundColor;
    chatterino::rj::getSafe(value, "backgroundColor", tmpBackgroundColor);
    if (this->backgroundColor)
    {
        *this->backgroundColor = QColor{tmpBackgroundColor};
    }
    else
    {
        this->backgroundColor = std::make_shared<QColor>(tmpBackgroundColor);
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
