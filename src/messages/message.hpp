#pragma once

#include "messages/message.hpp"
#include "messages/messageparseargs.hpp"
#include "messages/word.hpp"
#include "messages/wordpart.hpp"

#include <IrcMessage>
#include <QVector>

#include <chrono>
#include <memory>

namespace chatterino {

class Channel;

namespace messages {
class Message;

typedef std::shared_ptr<Message> SharedMessage;

class Message
{
public:
    // explicit Message(const QString &text);
    // explicit Message(const QString &text, const std::vector<messages::Word> &words,
    //                 const bool &highlight);

    bool getCanHighlightTab() const;
    const QString &getTimeoutUser() const;
    int getTimeoutCount() const;
    const QString &getUserName() const;
    const QString &getDisplayName() const;
    const QString &getContent() const;
    const std::chrono::time_point<std::chrono::system_clock> &getParseTime() const;
    std::vector<Word> &getWords();
    bool isDisabled() const;
    const QString &getId() const;

    const QString text;
    bool centered = false;

private:
    static LazyLoadedImage *badgeStaff;
    static LazyLoadedImage *badgeAdmin;
    static LazyLoadedImage *badgeGlobalmod;
    static LazyLoadedImage *badgeModerator;
    static LazyLoadedImage *badgeTurbo;
    static LazyLoadedImage *badgeBroadcaster;
    static LazyLoadedImage *badgePremium;

    static QRegularExpression *cheerRegex;

    bool highlightTab = false;
    QString timeoutUser = "";
    int timeoutCount = 0;
    bool disabled = false;
    std::chrono::time_point<std::chrono::system_clock> parseTime;

    QString userName = "";
    QString displayName = "";
    QString content;
    QString id = "";

    std::vector<Word> words;
};

}  // namespace messages
}  // namespace chatterino
