#pragma once

#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <QColor>
#include <QString>
#include <QUrl>

#include <memory>

namespace chatterino {

class Badge;

class HighlightBadge
{
public:
    bool operator==(const HighlightBadge &other) const;

    HighlightBadge(const QString &badgeName, const QString &displayName,
                   bool showInMentions, bool hasAlert, bool hasSound,
                   const QString &soundUrl, QColor color);

    HighlightBadge(const QString &badgeName, const QString &displayName,
                   bool showInMentions, bool hasAlert, bool hasSound,
                   const QString &soundUrl, std::shared_ptr<QColor> color);

    const QString &badgeName() const;
    const QString &displayName() const;
    bool showInMentions() const;
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

    /*
     * XXX: Use the constexpr constructor here once we are building with
     * Qt>=5.13.
     */
    static QColor FALLBACK_HIGHLIGHT_COLOR;

private:
    bool compare(const QString &id, const Badge &badge) const;

    QString badgeName_;
    QString displayName_;
    bool showInMentions_;
    bool hasAlert_;
    bool hasSound_;
    QUrl soundUrl_;
    std::shared_ptr<QColor> color_;

    bool isMulti_;
    bool hasVersions_;
    QStringList badges_;
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
        chatterino::rj::set(ret, "displayName", value.displayName(), a);
        chatterino::rj::set(ret, "showInMentions", value.showInMentions(), a);
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
    static chatterino::HighlightBadge get(const rapidjson::Value &value,
                                          bool *error)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error);
            return chatterino::HighlightBadge(QString(), QString(), false,
                                              false, false, "", QColor());
        }

        QString _name;
        QString _displayName;
        bool _showInMentions = false;
        bool _hasAlert = true;
        bool _hasSound = false;
        QString _soundUrl;
        QString encodedColor;

        chatterino::rj::getSafe(value, "name", _name);
        chatterino::rj::getSafe(value, "displayName", _displayName);
        chatterino::rj::getSafe(value, "showInMentions", _showInMentions);
        chatterino::rj::getSafe(value, "alert", _hasAlert);
        chatterino::rj::getSafe(value, "sound", _hasSound);
        chatterino::rj::getSafe(value, "soundUrl", _soundUrl);
        chatterino::rj::getSafe(value, "color", encodedColor);

        auto _color = QColor(encodedColor);
        if (!_color.isValid())
        {
            _color = chatterino::HighlightBadge::FALLBACK_HIGHLIGHT_COLOR;
        }

        return chatterino::HighlightBadge(_name, _displayName, _showInMentions,
                                          _hasAlert, _hasSound, _soundUrl,
                                          _color);
    }
};

}  // namespace pajlada
