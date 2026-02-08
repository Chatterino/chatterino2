// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QRegularExpression>
#include <QString>
#include <QSyntaxHighlighter>

#include <concepts>
#include <memory>

class QTextDocument;

namespace chatterino {

class Channel;
class TwitchChannel;
class SpellChecker;

namespace inputhighlight::detail {

QRegularExpression wordRegex();

}  // namespace inputhighlight::detail

/// This highlights the text in the split input.
/// Currently, it only does spell checking.
class InputHighlighter : public QSyntaxHighlighter
{
public:
    InputHighlighter(SpellChecker &spellChecker, QObject *parent);
    ~InputHighlighter() override;
    InputHighlighter(const InputHighlighter &) = delete;
    InputHighlighter(InputHighlighter &&) = delete;
    InputHighlighter &operator=(const InputHighlighter &) = delete;
    InputHighlighter &operator=(InputHighlighter &&) = delete;

    void setChannel(const std::shared_ptr<Channel> &channel);

    /// Do a pass over the whole text and filter out all words that will be
    /// checked by the spell checker.
    std::vector<QString> getSpellCheckedWords(const QString &text);

protected:
    void highlightBlock(const QString &text) override;

private:
    /// Visit all words that are not ignored
    void visitWords(
        const QString &text,
        std::invocable</*word=*/const QString &, /*start=*/qsizetype,
                       /*count=*/qsizetype> auto &&cb);

    SpellChecker &spellChecker;
    QTextCharFormat spellFmt;

    std::weak_ptr<TwitchChannel> channel;

    QRegularExpression wordRegex;
    QRegularExpression tokenRegex;
};

}  // namespace chatterino
