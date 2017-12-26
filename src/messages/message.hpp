#pragma once

#include "messages/word.hpp"

#include <chrono>
#include <memory>
#include <vector>

namespace chatterino {

class Channel;

namespace messages {
class Message;

typedef std::shared_ptr<Message> SharedMessage;

class Message
{
public:
    bool containsHighlightedPhrase() const;
    void setHighlight(bool value);
    const QString &getTimeoutUser() const;
    int getTimeoutCount() const;
    const QString &getContent() const;
    const std::chrono::time_point<std::chrono::system_clock> &getParseTime() const;
    std::vector<Word> &getWords();
    bool isDisabled() const;
    const QString &getId() const;
    bool getCollapsedDefault() const;
    void setCollapsedDefault(bool value);

    QString loginName;
    QString displayName;
    QString localizedName;

    const QString text;
    bool centered = false;

    static Message *createSystemMessage(const QString &text);

    static Message *createTimeoutMessage(const QString &username, const QString &durationInSeconds,
                                         const QString &reason);

private:
    static LazyLoadedImage *badgeStaff;
    static LazyLoadedImage *badgeAdmin;
    static LazyLoadedImage *badgeGlobalmod;
    static LazyLoadedImage *badgeModerator;
    static LazyLoadedImage *badgeTurbo;
    static LazyLoadedImage *badgeBroadcaster;
    static LazyLoadedImage *badgePremium;

    static QRegularExpression *cheerRegex;

    // what is highlightTab?
    bool highlightTab = false;

    QString timeoutUser = "";
    int timeoutCount = 0;
    bool disabled = false;

    bool collapsedDefault = false;

    std::chrono::time_point<std::chrono::system_clock> parseTime;

    QString content;
    QString id = "";

    std::vector<Word> words;
};

}  // namespace messages
}  // namespace chatterino
