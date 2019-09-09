#include "SearchPopup.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "messages/search/AuthorPredicate.hpp"
#include "messages/search/LinkPredicate.hpp"
#include "messages/search/SubstringPredicate.hpp"
#include "widgets/helper/ChannelView.hpp"

namespace chatterino {

ChannelPtr SearchPopup::filter(const QString &text, const QString &channelName,
                               const LimitedQueueSnapshot<MessagePtr> &snapshot)
{
    ChannelPtr channel(new Channel(channelName, Channel::Type::None));

    // Parse predicates from tags in "text"
    auto predicates = parsePredicates(text);

    // Check for every message whether it fulfills all predicates that have
    // been registered
    for (size_t i = 0; i < snapshot.size(); ++i)
    {
        MessagePtr message = snapshot[i];

        bool accept = true;
        for (MessagePredicatePtr &pred : predicates)
        {
            // Discard the message as soon as one predicate fails
            if (!pred->appliesTo(message))
            {
                accept = false;
                break;
            }
        }

        // If all predicates match, add the message to the channel
        if (accept)
            channel->addMessage(message);
    }

    return channel;
}

SearchPopup::SearchPopup()
{
    this->initLayout();
    this->resize(400, 600);
}

void SearchPopup::setChannel(const ChannelPtr &channel)
{
    this->channelName_ = channel->getName();
    this->snapshot_ = channel->getMessageSnapshot();
    this->search();

    this->updateWindowTitle();
}

void SearchPopup::updateWindowTitle()
{
    this->setWindowTitle("Searching in " + this->channelName_ + "s history");
}

void SearchPopup::search()
{
    this->channelView_->setChannel(filter(this->searchInput_->text(),
                                          this->channelName_, this->snapshot_));
}

void SearchPopup::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape)
    {
        this->close();
        return;
    }

    BaseWidget::keyPressEvent(e);
}

void SearchPopup::initLayout()
{
    // VBOX
    {
        QVBoxLayout *layout1 = new QVBoxLayout(this);
        layout1->setMargin(0);
        layout1->setSpacing(0);

        // HBOX
        {
            QHBoxLayout *layout2 = new QHBoxLayout(this);
            layout2->setMargin(8);
            layout2->setSpacing(8);

            // SEARCH INPUT
            {
                this->searchInput_ = new QLineEdit(this);
                layout2->addWidget(this->searchInput_);
                QObject::connect(this->searchInput_, &QLineEdit::returnPressed,
                                 [this] { this->search(); });
            }

            // SEARCH BUTTON
            {
                QPushButton *searchButton = new QPushButton(this);
                searchButton->setText("Search");
                layout2->addWidget(searchButton);
                QObject::connect(searchButton, &QPushButton::clicked,
                                 [this] { this->search(); });
            }

            layout1->addLayout(layout2);
        }

        // CHANNELVIEW
        {
            this->channelView_ = new ChannelView(this);

            layout1->addWidget(this->channelView_);
        }

        this->setLayout(layout1);
    }
}

std::vector<std::unique_ptr<MessagePredicate>> SearchPopup::parsePredicates(
    const QString &input)
{
    std::vector<std::unique_ptr<MessagePredicate>> predicates;

    // Get a working copy we can modify
    QString text = input;

    // Check for "from:" tags
    QStringList searchedUsers = parseSearchedUsers(text);
    if (searchedUsers.size() > 0)
    {
        predicates.push_back(std::make_unique<AuthorPredicate>(searchedUsers));
        removeTagFromText("from:", text);
    }

    // Check for "contains:link" tags
    if (text.contains("contains:link", Qt::CaseInsensitive))
    {
        predicates.push_back(std::make_unique<LinkPredicate>());
        removeTagFromText("contains:link", text);
    }

    // The rest of the input is treated as a substring search.
    // If "text" is empty, every message will be matched.
    if (text.size() > 0)
    {
        predicates.push_back(std::make_unique<SubstringPredicate>(text));
    }

    return predicates;
}

void SearchPopup::removeTagFromText(const QString &tag, QString &text)
{
    for (QString &word : text.split(' ', QString::SkipEmptyParts))
    {
        if (word.startsWith(tag, Qt::CaseInsensitive))
            text.remove(word);
    }

    // Remove whitespace introduced by spaces between tags
    text = text.trimmed();
}

QStringList SearchPopup::parseSearchedUsers(const QString &input)
{
    QStringList parsedUserNames;

    for (QString &word : input.split(' ', QString::SkipEmptyParts))
    {
        // Users can be searched for by specifying them like so:
        // "from:user1,user2,user3" or "from:user1 from:user2 from:user3"

        if (!word.startsWith("from:"))
            // Ignore this word
            continue;

        // Get a working copy so we can manipulate the string
        QString fromTag = word;

        // Delete the "from:" part so we can parse the user names more easily
        fromTag.remove(0, 5);

        // Parse comma-seperated user names
        for (QString &user : fromTag.split(',', QString::SkipEmptyParts))
        {
            parsedUserNames << user;
        }
    }

    return parsedUserNames;
}

}  // namespace chatterino
