// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QColor>
#include <QString>

namespace chatterino {

struct MessageColors;

struct MessageColor {
    enum Type {
        Custom,
        Text,
        Link,
        System,
        /// Behaves like Custom, but marks the color as a user's username
        /// color (e.g. their Twitch color), so it can be exempt from
        /// readability adjustments
        Username,
    };

    MessageColor(const QColor &color);
    MessageColor(Type type_ = Text);

    /// Creates a color that is marked as a user's username color
    static MessageColor username(const QColor &color);

    const QColor &getColor(const MessageColors &colors) const;

    /// Returns a copy of this color marked as a username color, if it's a
    /// custom color. Other colors are returned unchanged.
    MessageColor asUsernameColor() const;

    bool isUsernameColor() const noexcept
    {
        return this->type_ == Type::Username;
    }

    QString toString() const;

    QString toLua() const;
    static MessageColor fromLua(const QString &spec,
                                Type fallback = Type::Text);

    bool operator==(const MessageColor &other) const noexcept
    {
        return this->type_ == other.type_ &&
               this->customColor_ == other.customColor_;
    }

private:
    Type type_;
    QColor customColor_;
};

}  // namespace chatterino
