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
#include "util/Shortcut.hpp"
#include "widgets/helper/ChannelView.hpp"

namespace chatterino {

ChannelPtr SearchPopup::filter(const QString &text, const QString &channelName,
                               const LimitedQueueSnapshot<MessagePtr> &snapshot,
                               FilterSet *filterSet)
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
        for (const auto &pred : predicates)
        {
            // Discard the message as soon as one predicate fails
            if (!pred->appliesTo(*message))
            {
                accept = false;
                break;
            }
        }

        if (accept && filterSet != nullptr)
            accept = filterSet->filter(message);

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

    createShortcut(this, "CTRL+F", [this] {
        this->searchInput_->setFocus();
        this->searchInput_->selectAll();
    });
}

void SearchPopup::setChannelFilters(FilterSet *filters)
{
    this->channelFilters_ = filters;
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
                                          this->channelName_, this->snapshot_,
                                          this->channelFilters_));
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
    static QRegularExpression predicateRegex(R"(^(\w+):([\w,]+)$)");

    auto predicates = std::vector<std::unique_ptr<MessagePredicate>>();
    auto words = input.split(' ', QString::SkipEmptyParts);
    auto authors = QStringList();

    for (auto it = words.begin(); it != words.end();)
    {
        if (auto match = predicateRegex.match(*it); match.hasMatch())
        {
            QString name = match.captured(1);
            QString value = match.captured(2);

            bool remove = true;

            // match predicates
            if (name == "from")
            {
                authors.append(value);
            }
            else if (name == "has" && value == "link")
            {
                predicates.push_back(std::make_unique<LinkPredicate>());
            }
            else
            {
                remove = false;
            }

            // remove or advance
            it = remove ? words.erase(it) : ++it;
        }
        else
        {
            ++it;
        }
    }

    if (!authors.empty())
        predicates.push_back(std::make_unique<AuthorPredicate>(authors));

    if (!words.empty())
        predicates.push_back(
            std::make_unique<SubstringPredicate>(words.join(" ")));

    return predicates;
}

}  // namespace chatterino
