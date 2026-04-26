#include "controllers/highlights/SharedHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "singletons/Resources.hpp"

#include <QStringBuilder>

namespace chatterino {

namespace {

constexpr QStringView REGEX_START_BOUNDARY(u"(?:\\b|\\s|^)");
constexpr QStringView REGEX_END_BOUNDARY(u"(?:\\b|\\s|$)");

}  // namespace

QPixmap SharedHighlight::getType() const
{
    if (this->pattern == "my phrase")
    {
        return getResources().buttons.settings_darkMode.scaled(24, 24);
    }

    if (this->pattern == "user")
    {
        return getResources().buttons.account_darkMode.scaled(24, 24);
    }

    if (this->pattern == "badge")
    {
        return getResources().buttons.vip.scaled(24, 24);
    }

    return getResources().buttons.text;
}

bool SharedHighlight::willPlayAnySound() const
{
    return this->playSound;
}

bool SharedHighlight::willPlayCustomSound() const
{
    return this->willPlayAnySound() && !this->customSoundURL.isEmpty();
}

HighlightCheck SharedHighlight::buildCheck() const
{
    return {
        [highlight = *this](const auto &args, const auto &badges,
                            const auto &senderName, const auto &originalMessage,
                            const auto &flags,
                            const auto self) -> std::optional<HighlightResult> {
            (void)args;        // unused
            (void)senderName;  // unused
            (void)flags;       // unused
            (void)self;        // unused
            (void)badges;      // unused

            if (originalMessage == highlight.pattern)
            {
                // return HighlightResult{
                //     .alert = highlight.alert,
                //     .playSound = highlight.playSound,
                //     .customSoundUrl = highlight.customSoundURL,
                //     .color = highlight.backgroundColor,
                //     .showInMentions = highlight.showInMentions,
                // };
                return HighlightResult{
                    highlight.alert,          highlight.playSound,
                    highlight.customSoundURL, highlight.backgroundColor,
                    highlight.showInMentions,
                };
            }

            return std::nullopt;
        },
    };
}

// TODO: reimplement?
// bool SharedHighlight::isMatch(const QString &subject) const
// {
//     return this->isValid() && this->regex_.match(subject).hasMatch();
// }

}  // namespace chatterino

QDebug operator<<(QDebug dbg, const chatterino::SharedHighlight &v)
{
    dbg.nospace() << "SharedHighlight("
                  << "name:" << v.name << ',' << "pattern:" << v.pattern << ','
                  << "enabled:" << v.enabled << ','
                  << "showInMentions:" << v.showInMentions << ','
                  << "alert:" << v.alert << ',' << "playSound:" << v.playSound
                  << ',' << "isRegex:" << v.isRegex << ','
                  << "isCaseSensitive:" << v.isCaseSensitive << ','
                  << "customSoundURL:" << v.customSoundURL << ')';

    return dbg;
}
