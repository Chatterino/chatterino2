#pragma once

#include "ForwardDecl.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "messages/search/MessagePredicate.hpp"
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

    /**
     * @brief Only retains those message from a list of messages that satisfy a
     *        search query.
     *
     * @param text          the search query -- will be parsed for MessagePredicates
     * @param channelName   name of the channel to be returned
     * @param snapshot      list of messages to filter
     *
     * @return a ChannelPtr with "channelName" and the filtered messages from
     *         "snapshot"
     */
    static ChannelPtr filter(const QString &text, const QString &channelName,
                             const LimitedQueueSnapshot<MessagePtr> &snapshot);

    /**
     * @brief Checks the input for tags and registers their corresponding
     *        predicates.
     *
     * @param input the string to check for tags
     * @return a vector of MessagePredicates requested in the input
     */
    static std::vector<std::unique_ptr<MessagePredicate>> parsePredicates(
        const QString &input);

    LimitedQueueSnapshot<MessagePtr> snapshot_;
    QLineEdit *searchInput_{};
    ChannelView *channelView_{};
    QString channelName_{};
};

}  // namespace chatterino
