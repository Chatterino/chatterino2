#pragma once

#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QColor>
#include <QRegularExpression>
#include <QString>
#include <QUrl>

#include <memory>

namespace chatterino {

class HighlightPhrase
{
public:
    bool operator==(const HighlightPhrase &other) const;

    /**
     * @brief Create a new HighlightPhrase.
     *
     * Use this constructor when creating a new HighlightPhrase.
     */
    HighlightPhrase(const QString &pattern, bool showInMentions, bool hasAlert,
                    bool hasSound, bool isRegex, bool isCaseSensitive,
                    const QString &soundUrl, QColor color);

    /**
     * @brief Create a new HighlightPhrase.
     *
     * Use this constructor when updating an existing HighlightPhrase's color.
     */
    HighlightPhrase(const QString &pattern, bool showInMentions, bool hasAlert,
                    bool hasSound, bool isRegex, bool isCaseSensitive,
                    const QString &soundUrl, std::shared_ptr<QColor> color);

    const QString &getPattern() const;
    bool showInMentions() const;
    bool hasAlert() const;

    /**
     * @brief Check if this highlight phrase should play a sound when
     *        triggered.
     *
     * In distinction from `HighlightPhrase::hasCustomSound`, this method only
     * checks whether or not ANY sound should be played when the phrase is
     * triggered.
     * 
     * To check whether a custom sound is set, use
     * `HighlightPhrase::hasCustomSound` instead.
     *
     * @return true, if this highlight phrase should play a sound when
     *         triggered, false otherwise
     */
    bool hasSound() const;

    /**
     * @brief Check if this highlight phrase has a custom sound set.
     *
     * Note that this method only checks whether the path to the custom sound
     * is not empty. It does not check whether the file still exists, is a
     * sound file, or anything else.
     *
     * @return true, if the custom sound file path is not empty, false otherwise
     */
    bool hasCustomSound() const;

    bool isRegex() const;
    bool isValid() const;
    bool isMatch(const QString &subject) const;
    bool isCaseSensitive() const;
    const QUrl &getSoundUrl() const;
    const std::shared_ptr<QColor> getColor() const;

    /*
     * XXX: Use the constexpr constructor here once we are building with
     * Qt>=5.13.
     */
    static QColor FALLBACK_HIGHLIGHT_COLOR;
    // Used for automatic self messages highlighing
    static QColor FALLBACK_SELF_MESSAGE_HIGHLIGHT_COLOR;
    static QColor FALLBACK_REDEEMED_HIGHLIGHT_COLOR;
    static QColor FALLBACK_SUB_COLOR;
    static QColor FALLBACK_FIRST_MESSAGE_HIGHLIGHT_COLOR;
    static QColor FALLBACK_ELEVATED_MESSAGE_HIGHLIGHT_COLOR;
    static QColor FALLBACK_THREAD_HIGHLIGHT_COLOR;
    static QColor FALLBACK_AUTOMOD_HIGHLIGHT_COLOR;

private:
    QString pattern_;
    bool showInMentions_;
    bool hasAlert_;
    bool hasSound_;
    bool isRegex_;
    bool isCaseSensitive_;
    QUrl soundUrl_;
    std::shared_ptr<QColor> color_;
    QRegularExpression regex_;
};

}  // namespace chatterino

namespace pajlada {

namespace {
    chatterino::HighlightPhrase constructError()
    {
        return chatterino::HighlightPhrase(QString(), false, false, false,
                                           false, false, QString(), QColor());
    }
}  // namespace

template <>
struct Serialize<chatterino::HighlightPhrase> {
    static rapidjson::Value get(const chatterino::HighlightPhrase &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "pattern", value.getPattern(), a);
        chatterino::rj::set(ret, "showInMentions", value.showInMentions(), a);
        chatterino::rj::set(ret, "alert", value.hasAlert(), a);
        chatterino::rj::set(ret, "sound", value.hasSound(), a);
        chatterino::rj::set(ret, "regex", value.isRegex(), a);
        chatterino::rj::set(ret, "case", value.isCaseSensitive(), a);
        chatterino::rj::set(ret, "soundUrl", value.getSoundUrl().toString(), a);
        chatterino::rj::set(ret, "color",
                            value.getColor()->name(QColor::HexArgb), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::HighlightPhrase> {
    static chatterino::HighlightPhrase get(const rapidjson::Value &value,
                                           bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)

            return constructError();
        }

        QString _pattern;
        bool _showInMentions = true;
        bool _hasAlert = true;
        bool _hasSound = false;
        bool _isRegex = false;
        bool _isCaseSensitive = false;
        QString _soundUrl;
        QString encodedColor;

        chatterino::rj::getSafe(value, "pattern", _pattern);
        chatterino::rj::getSafe(value, "showInMentions", _showInMentions);
        chatterino::rj::getSafe(value, "alert", _hasAlert);
        chatterino::rj::getSafe(value, "sound", _hasSound);
        chatterino::rj::getSafe(value, "regex", _isRegex);
        chatterino::rj::getSafe(value, "case", _isCaseSensitive);
        chatterino::rj::getSafe(value, "soundUrl", _soundUrl);
        chatterino::rj::getSafe(value, "color", encodedColor);

        auto _color = QColor(encodedColor);
        if (!_color.isValid())
        {
            _color = chatterino::HighlightPhrase::FALLBACK_HIGHLIGHT_COLOR;
        }

        return chatterino::HighlightPhrase(_pattern, _showInMentions, _hasAlert,
                                           _hasSound, _isRegex,
                                           _isCaseSensitive, _soundUrl, _color);
    }
};

}  // namespace pajlada
