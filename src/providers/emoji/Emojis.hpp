// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "common/FlagsEnum.hpp"
#include "providers/emoji/EmojiStyle.hpp"

#include <QMap>
#include <QRegularExpression>
#include <QVector>

#include <memory>
#include <variant>
#include <vector>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

struct EmojiData {
    // actual byte-representation of the emoji (i.e. \154075\156150 which is
    // :male:)
    QString value;

    // actual byte-representation of the non qualified emoji
    QString nonQualified;

    // i.e. 204e-50a2
    QString unifiedCode;
    QString nonQualifiedCode;

    // i.e. thinking
    std::vector<QString> shortCodes;

    QString category;

    using Capability = EmojiStyle;
    using Capabilities = FlagsEnum<Capability>;

    Capabilities capabilities;

    std::vector<EmojiData> variations;

    EmotePtr emote;
};

using EmojiPtr = std::shared_ptr<EmojiData>;

class IEmojis
{
public:
    virtual ~IEmojis() = default;

    virtual std::vector<std::variant<EmotePtr, QStringView>> parse(
        QStringView text) const = 0;
    virtual const std::vector<EmojiPtr> &getEmojis() const = 0;
    virtual const std::vector<QString> &getShortCodes() const = 0;
    virtual QString replaceShortCodes(const QString &text) const = 0;
};

class Emojis : public IEmojis
{
public:
    void load();
    std::vector<std::variant<EmotePtr, QStringView>> parse(
        QStringView text) const override;

    std::vector<QString> shortCodes;
    QString replaceShortCodes(const QString &text) const override;

    const std::vector<EmojiPtr> &getEmojis() const override;
    const std::vector<QString> &getShortCodes() const override;

private:
    void loadEmojis();
    void sortEmojis();
    void loadEmojiSet();

    std::vector<EmojiPtr> emojis;

    /// Emojis
    QRegularExpression findShortCodesRegex_{":([-+\\w]+):"};

    // shortCodeToEmoji maps strings like "sunglasses" to its emoji
    QMap<QString, std::shared_ptr<EmojiData>> emojiShortCodeToEmoji_;

    // Maps the first character of the emoji unicode string to a vector of
    // possible emojis
    QMap<QChar, QVector<std::shared_ptr<EmojiData>>> emojiFirstByte_;

    bool loaded_ = false;
};

}  // namespace chatterino
