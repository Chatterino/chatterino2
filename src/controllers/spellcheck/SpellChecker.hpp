#pragma once

#include <QRegularExpression>
#include <QSyntaxHighlighter>

#include <memory>
#include <string>
#include <vector>

class QTextDocument;

namespace chatterino {

class Channel;
class TwitchChannel;

class SpellCheckerPrivate;
class SpellChecker
{
public:
    SpellChecker();
    ~SpellChecker();

    bool isLoaded() const;
    void reload();

    bool check(const QString &word);
    std::vector<std::string> suggestions(const QString &word);

private:
    std::unique_ptr<SpellCheckerPrivate> private_;
};

class SpellCheckHighlighter : public QSyntaxHighlighter
{
public:
    SpellCheckHighlighter(QObject *parent);

    void setChannel(const std::shared_ptr<Channel> &channel);

protected:
    void highlightBlock(const QString &text) override;

private:
    QRegularExpression wordRegex;
    QTextCharFormat spellFmt;
    std::weak_ptr<TwitchChannel> channel;
};

}  // namespace chatterino
