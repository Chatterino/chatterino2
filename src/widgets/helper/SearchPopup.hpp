#pragma once

#include "ForwardDecl.hpp"
#include "messages/LimitedQueueSnapshot.hpp"
#include "widgets/BasePopup.hpp"

#include <memory>

class QLineEdit;

namespace chatterino {

class Split;
class MessagePredicate;

class SearchPopup : public BasePopup
{
public:
    SearchPopup(QWidget *parent, Split *split = nullptr);

    virtual void addChannel(ChannelView &channel);
    void goToMessage(const MessagePtr &message);
    /**
     * This method should only be used for searches that
     * don't include a mentions channel,
     * since it will only search in the opened channels (not globally).
     * @param messageId
     */
    void goToMessageId(const QString &messageId);

protected:
    virtual void updateWindowTitle();
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void themeChangedEvent() override;

private:
    void initLayout();
    void search();
    void addShortcuts() override;
    LimitedQueueSnapshot<MessagePtr> buildSnapshot();

    /**
     * @brief Only retains those message from a list of messages that satisfy a
     *        search query.
     *
     * @param text          the search query -- will be parsed for MessagePredicates
     * @param channelName   name of the channel to be returned
     * @param snapshot      list of messages to filter
     * @param filterSet     channel filter to apply
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
    Split *split_ = nullptr;
    QList<std::reference_wrapper<ChannelView>> searchChannels_;
};

}  // namespace chatterino
