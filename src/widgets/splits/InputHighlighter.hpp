#pragma once

#include <QRegularExpression>
#include <QString>
#include <QSyntaxHighlighter>

#include <memory>

class QTextDocument;

namespace chatterino {

class Channel;
class TwitchChannel;
class SpellChecker;

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

protected:
    void highlightBlock(const QString &text) override;

private:
    SpellChecker &spellChecker;
    QTextCharFormat spellFmt;

    std::weak_ptr<TwitchChannel> channel;

    QRegularExpression wordRegex;
};

}  // namespace chatterino
