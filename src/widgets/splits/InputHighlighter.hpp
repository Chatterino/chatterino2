#pragma once

#include <QSyntaxHighlighter>

namespace chatterino {

class Channel;
class TwitchChannel;
class SpellChecker;

/// This highlights the text in the split input.
/// Currently, it only does syntax highlighting.
class InputHighlighter : public QSyntaxHighlighter
{
public:
    InputHighlighter(SpellChecker &spellChecker, QObject *parent);
    ~InputHighlighter() override;

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
