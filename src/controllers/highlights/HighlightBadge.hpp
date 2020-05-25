#pragma once

#include "providers/twitch/TwitchBadge.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "util/RapidjsonHelpers.hpp"

#include <QString>
#include <pajlada/serialize.hpp>

namespace chatterino {
class HighlightBadge
{
public:
    bool operator==(const HighlightBadge &other) const;

    HighlightBadge(const QString &badgeName, const QString &badgeVersion,
                   const QString &displayName, bool hasAlert, bool hasSound,
                   const QString &soundUrl, QColor color);

    HighlightBadge(const QString &badgeName, const QString &badgeVersion,
                   const QString &displayName, bool hasAlert, bool hasSound,
                   const QString &soundUrl, std::shared_ptr<QColor> color);

    const QString &badgeName() const;
    const QString &badgeVersion() const;
    const QString &displayName() const;
    bool hasAlert() const;
    bool hasSound() const;
    bool isMatch(const Badge &badge) const;

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

    const QUrl &getSoundUrl() const;
    const std::shared_ptr<QColor> getColor() const;

    QString identifier() const;

    /*
     * XXX: Use the constexpr constructor here once we are building with
     * Qt>=5.13.
     */
    static QColor FALLBACK_HIGHLIGHT_COLOR;

private:
    QString badgeName_;
    QString badgeVersion_;
    QString displayName_;
    bool hasAlert_;
    bool hasSound_;
    QUrl soundUrl_;
    std::shared_ptr<QColor> color_;
};
};  // namespace chatterino

namespace pajlada {

template <>
struct Serialize<chatterino::HighlightBadge> {
    static rapidjson::Value get(const chatterino::HighlightBadge &value,
                                rapidjson::Document::AllocatorType &a)
    {
        rapidjson::Value ret(rapidjson::kObjectType);

        chatterino::rj::set(ret, "name", value.badgeName(), a);
        chatterino::rj::set(ret, "version", value.badgeVersion(), a);
        chatterino::rj::set(ret, "displayName", value.displayName(), a);
        chatterino::rj::set(ret, "alert", value.hasAlert(), a);
        chatterino::rj::set(ret, "sound", value.hasSound(), a);
        chatterino::rj::set(ret, "soundUrl", value.getSoundUrl().toString(), a);
        chatterino::rj::set(ret, "color",
                            value.getColor()->name(QColor::HexArgb), a);

        return ret;
    }
};

template <>
struct Deserialize<chatterino::HighlightBadge> {
    static chatterino::HighlightBadge get(const rapidjson::Value &value)
    {
        if (!value.IsObject())
        {
            return chatterino::HighlightBadge(QString(), QString(), QString(),
                                              false, false, "", QColor());
        }

        QString _name;
        QString _version;
        QString _displayName;
        bool _hasAlert = true;
        bool _hasSound = false;
        QString _soundUrl;
        QString encodedColor;

        chatterino::rj::getSafe(value, "name", _name);
        chatterino::rj::getSafe(value, "version", _version);
        chatterino::rj::getSafe(value, "displayName", _displayName);
        chatterino::rj::getSafe(value, "alert", _hasAlert);
        chatterino::rj::getSafe(value, "sound", _hasSound);
        chatterino::rj::getSafe(value, "soundUrl", _soundUrl);
        chatterino::rj::getSafe(value, "color", encodedColor);

        auto _color = QColor(encodedColor);
        if (!_color.isValid())
            _color = chatterino::HighlightBadge::FALLBACK_HIGHLIGHT_COLOR;

        return chatterino::HighlightBadge(_name, _version, _displayName,
                                          _hasAlert, _hasSound, _soundUrl,
                                          _color);
    }
};

}  // namespace pajlada
