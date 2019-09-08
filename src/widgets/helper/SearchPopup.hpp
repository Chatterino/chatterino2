#pragma once

#include "ForwardDecl.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/predicates/MessagePredicate.hpp"
#include "widgets/BaseWindow.hpp"

#include <memory>

class QLineEdit;

namespace chatterino {

class SearchPopup : public BaseWindow
{
public:
    SearchPopup();

    virtual void setChannel(const ChannelPtr &channel);

protected:
    void keyPressEvent(QKeyEvent *e) override;

    virtual void updateWindowTitle();

private:
    void initLayout();
    void search();

    static ChannelPtr filter(const QString &text, const QString &channelName, const LimitedQueueSnapshot<MessagePtr> &snapshot);

    /**
     * @brief Checks the input for tags and registers their corresponding
     *        predicates.
     *
     * @param input the string to check for tags
     */
    static std::vector<MessagePredicatePtr> parsePredicates(const QString& input);

    /**
     * @brief Removes every occurence of a tag from the passed string.
     *
     * @param tag the tag prefix to search for
     * @param text the text to remove the tags from
     */
    static void removeTagFromText(const QString& tag, QString& text);

    /**
     * @brief Parses a comma-seperated list of user names given by "from:"
     *        tags.
     *
     * Users can be searched for by specifying them like so:
     * "from:user1,user2,user3" or "from:user1 from:user2 from:user3".
     *
     * @param input the message to search for "from:" tags in
     * @return a list of user names passed after "from:" tags
     */
    static QStringList parseSearchedUsers(const QString& input);

    LimitedQueueSnapshot<MessagePtr> snapshot_;
    QLineEdit *searchInput_;
    ChannelView *channelView_;
    QString channelName_;
};

}  // namespace chatterino
