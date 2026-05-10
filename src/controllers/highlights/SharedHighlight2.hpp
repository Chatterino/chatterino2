// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/highlights/types/Common.hpp"
#include "controllers/highlights/types/Outcome.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <pajlada/serialize.hpp>
#include <pajlada/serialize/common.hpp>
#include <pajlada/serialize/deserialize.hpp>
#include <pajlada/serialize/serialize.hpp>
#include <QColor>
#include <QDebug>
#include <QPixmap>
#include <QRegularExpression>
#include <QString>
#include <QStringView>
#include <QUrl>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cassert>
#include <memory>
#include <optional>

namespace chatterino {

struct HighlightCheck;

}  // namespace chatterino

namespace chatterino::highlights {

struct SharedHighlight2 {
    bool operator==(const SharedHighlight2 &other) const = default;

    SharedHighlight2() = default;
    virtual ~SharedHighlight2() = default;

    QString getPattern() const
    {
        return "no pattern";
    }

    /// Returns true if this highlight should be enabled.
    ///
    /// If unconfigured, returns true.
    virtual bool isEnabled() const;
    void setEnabled(std::optional<bool> newValue);

    /// Returns true if this highlight should show the message in /mentions.
    ///
    /// If unconfigured, returns true.
    virtual bool shouldShowInMentions() const;
    virtual void setShowInMentions(std::optional<bool> newValue);

    /// Returns true if this highlight should highlight the taskbar.
    ///
    /// If unconfigured, returns true.
    virtual bool shouldHighlightTaskbar() const;
    virtual void setHighlightTaskbar(std::optional<bool> newValue);

    /// Returns true if this highlight should be matched with a regex.
    ///
    /// If unconfigured, returns false.
    virtual bool isRegex() const;
    virtual void setRegex(std::optional<bool> newValue);

    /// Returns true if this highlight should be matched with case sensitivity.
    ///
    /// If unconfigured, returns false.
    virtual bool isCaseSensitive() const;
    virtual void setCaseSensitive(std::optional<bool> newValue);

    /// Returns true if this highlight should play a sound.
    ///
    /// If unconfigured, returns false.
    virtual bool shouldPlaySound() const;
    void setPlaySound(std::optional<bool> newValue);

    QUrl getSoundUrl() const;
    void setSoundUrl(const QUrl &newValue);

    std::shared_ptr<QColor> getBackgroundColor() const;
    void setBackgroundColor(const QColor &newValue);

    /// The display name/pretty name of this highlight.
    /// If empty, we will try to auto-generate something that makes sense (e.g. "Text contains 'foo'")
    QString name;

protected:
    std::optional<bool> enabled;

    [[deprecated]]
    static bool matchesID(const rapidjson::Value &value, QStringView expectedID)
    {
        return chatterino::highlights::matchesID(value, expectedID);
    }

public:
    /// Contains the highlight pattern.
    /// This can be a simple text match:
    ///  - "foo" would match if the message text contains "foo"
    ///  - "regex:^foo$" would match if the message text is exactly "foo"
    ///  - "cs:FoO" would match if the message text contains "FoO" (case sensitive)
    /// meta matchers
    ///  - "badge:sub" would match if the author has any sub badge
    ///  - "name:forsen" would match if the author's name is forsen
    /// or any combination of the above
    ///  - "badge:sub foo" would match if the user has any sub badge and their message text contains "foo"
    QString pattern;

protected:
    Outcome outcome;

    std::optional<bool> regex;
    std::optional<bool> caseSensitive;

public:
    QIcon getType() const;

    bool willPlayAnySound() const;
    bool willPlayCustomSound() const;

    HighlightCheck buildCheck() const;

protected:
    void serialize(rapidjson::Value &ret,
                   rapidjson::Document::AllocatorType &a) const
    {
        // error if id is not set, or use "invalid" or something
        // TODO: id should probably be set in some CustomHighlight struct instead
        // rj::set(ret, "id", this->id, a);

        rj::setOptionally(ret, "name", this->name, a);
        rj::setOptionally(ret, "enabled", this->enabled, a);
        rj::setOptionally(ret, "pattern", this->pattern, a);

        this->outcome.serialize(ret, a);

        rj::setOptionally(ret, "regex", this->regex, a);
        rj::setOptionally(ret, "caseSensitive", this->caseSensitive, a);
    }

    bool deserialize(const rapidjson::Value &value)
    {
        assert(value.IsObject());

        chatterino::rj::getSafe(value, "name", this->name);
        chatterino::rj::getSafe(value, "enabled", this->enabled);
        chatterino::rj::getSafe(value, "pattern", this->pattern);

        chatterino::rj::getSafe(value, "regex", this->regex);
        chatterino::rj::getSafe(value, "caseSensitive", this->caseSensitive);

        this->outcome.deserialize(value);

        return true;
    }

    template <typename Type, typename RJValue>
    friend struct pajlada::Serialize;

    template <typename Type, typename RJValue, typename Enable>
    friend struct pajlada::Deserialize;

    friend class ConfigureDialog;

    friend QDebug operator<<(QDebug dbg, const SharedHighlight2 &v);
};

}  // namespace chatterino::highlights

namespace pajlada {

template <>
struct Serialize<chatterino::highlights::SharedHighlight2> {
    static rapidjson::Value get(
        const chatterino::highlights::SharedHighlight2 &value,
        rapidjson::Document::AllocatorType &a)
    {
        using namespace chatterino;
        rapidjson::Value ret(rapidjson::kObjectType);
        value.serialize(ret, a);
        return ret;
    }
};

template <>
struct Deserialize<chatterino::highlights::SharedHighlight2> {
    static chatterino::highlights::SharedHighlight2 get(
        const rapidjson::Value &value, bool *error = nullptr)
    {
        if (!value.IsObject())
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        chatterino::highlights::SharedHighlight2 h;

        if (!h.deserialize(value))
        {
            PAJLADA_REPORT_ERROR(error)
            return {};
        }

        return h;
    }
};

}  // namespace pajlada
