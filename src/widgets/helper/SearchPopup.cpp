#include "SearchPopup.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>

#include "common/Channel.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "messages/search/AuthorPredicate.hpp"
#include "messages/search/ChannelPredicate.hpp"
#include "messages/search/LinkPredicate.hpp"
#include "messages/search/MessageFlagsPredicate.hpp"
#include "messages/search/RegexPredicate.hpp"
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
        for (const auto &pred : predicates)
        {
            // Discard the message as soon as one predicate fails
            if (!pred->appliesTo(*message))
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

SearchPopup::SearchPopup(QWidget *parent, Split *split)
    : BasePopup({}, parent)
    , split_(split)
{
    this->initLayout();
    this->resize(400, 600);
    this->addShortcuts();
}

void SearchPopup::addShortcuts()
{
    HotkeyController::HotkeyMap actions{
        {"search",
         [this](const std::vector<QString> &) -> QString {
             this->searchInput_->setFocus();
             this->searchInput_->selectAll();
             return "";
         }},
        {"delete",
         [this](const std::vector<QString> &) -> QString {
             this->close();
             return "";
         }},

        {"reject", nullptr},
        {"accept", nullptr},
        {"openTab", nullptr},
        {"scrollPage", nullptr},
    };

    this->shortcuts_ = getApp()->hotkeys->shortcutsForCategory(
        HotkeyCategory::PopupWindow, actions, this);
}

void SearchPopup::addChannel(ChannelView &channel)
{
    if (this->searchChannels_.empty())
    {
        this->channelView_->setSourceChannel(channel.channel());
        this->channelName_ = channel.channel()->getName();
    }
    else if (this->searchChannels_.size() == 1)
    {
        this->channelView_->setSourceChannel(
            std::make_shared<Channel>("multichannel", Channel::Type::None));

        auto flags = this->channelView_->getFlags();
        flags.set(MessageElementFlag::ChannelName);
        flags.unset(MessageElementFlag::ModeratorTools);
        this->channelView_->setOverrideFlags(flags);
    }

    this->searchChannels_.append(std::ref(channel));

    this->updateWindowTitle();
}

void SearchPopup::updateWindowTitle()
{
    QString historyName;

    if (this->searchChannels_.size() > 1)
    {
        historyName = "multiple channels'";
    }
    else if (this->channelName_ == "/mentions")
    {
        historyName = "mentions";
    }
    else if (this->channelName_ == "/whispers")
    {
        historyName = "whispers";
    }
    else if (this->channelName_.isEmpty())
    {
        historyName = "<empty>'s";
    }
    else
    {
        historyName = QString("%1's").arg(this->channelName_);
    }
    this->setWindowTitle("Searching in " + historyName + " history");
}

void SearchPopup::showEvent(QShowEvent *)
{
    this->search();
}

void SearchPopup::search()
{
    if (this->snapshot_.size() == 0)
    {
        this->snapshot_ = this->buildSnapshot();
    }

    this->channelView_->setChannel(filter(this->searchInput_->text(),
                                          this->channelName_, this->snapshot_));
}

LimitedQueueSnapshot<MessagePtr> SearchPopup::buildSnapshot()
{
    // no point in filtering/sorting if it's a single channel search
    if (this->searchChannels_.length() == 1)
    {
        const auto channelPtr = this->searchChannels_.at(0);
        return channelPtr.get().channel()->getMessageSnapshot();
    }

    auto combinedSnapshot = std::vector<std::shared_ptr<const Message>>{};
    for (auto &channel : this->searchChannels_)
    {
        ChannelView &sharedView = channel.get();

        const FilterSetPtr filterSet = sharedView.getFilterSet();
        const LimitedQueueSnapshot<MessagePtr> &snapshot =
            sharedView.channel()->getMessageSnapshot();

        // TODO: implement iterator on LimitedQueueSnapshot?
        for (auto i = 0; i < snapshot.size(); ++i)
        {
            const MessagePtr &message = snapshot[i];
            if (filterSet && !filterSet->filter(message, sharedView.channel()))
            {
                continue;
            }

            combinedSnapshot.push_back(message);
        }
    }

    // remove any duplicate messages from splits containing the same channel
    std::sort(combinedSnapshot.begin(), combinedSnapshot.end(),
              [](MessagePtr &a, MessagePtr &b) {
                  return a->id > b->id;
              });

    auto uniqueIterator =
        std::unique(combinedSnapshot.begin(), combinedSnapshot.end(),
                    [](MessagePtr &a, MessagePtr &b) {
                        // nullptr check prevents system messages from being dropped
                        return (a->id != nullptr) && a->id == b->id;
                    });

    combinedSnapshot.erase(uniqueIterator, combinedSnapshot.end());

    // resort by time for presentation
    std::sort(combinedSnapshot.begin(), combinedSnapshot.end(),
              [](MessagePtr &a, MessagePtr &b) {
                  return a->serverReceivedTime < b->serverReceivedTime;
              });

    auto queue = LimitedQueue<MessagePtr>(combinedSnapshot.size());
    queue.pushFront(combinedSnapshot);

    return queue.getSnapshot();
}

void SearchPopup::initLayout()
{
    // VBOX
    {
        auto *layout1 = new QVBoxLayout(this);
        layout1->setMargin(0);
        layout1->setSpacing(0);

        // HBOX
        {
            auto *layout2 = new QHBoxLayout(this);
            layout2->setMargin(8);
            layout2->setSpacing(8);

            // SEARCH INPUT
            {
                this->searchInput_ = new QLineEdit(this);
                layout2->addWidget(this->searchInput_);

                this->searchInput_->setPlaceholderText("Type to search");
                this->searchInput_->setClearButtonEnabled(true);
                this->searchInput_->findChild<QAbstractButton *>()->setIcon(
                    QPixmap(":/buttons/clearSearch.png"));
                QObject::connect(this->searchInput_, &QLineEdit::textChanged,
                                 this, &SearchPopup::search);
            }

            layout1->addLayout(layout2);
        }

        // CHANNELVIEW
        {
            this->channelView_ = new ChannelView(this, this->split_,
                                                 ChannelView::Context::Search);

            layout1->addWidget(this->channelView_);
        }

        this->setLayout(layout1);
    }

    this->searchInput_->setFocus();
}

std::vector<std::unique_ptr<MessagePredicate>> SearchPopup::parsePredicates(
    const QString &input)
{
    // This regex captures all name:value predicate pairs into named capturing
    // groups and matches all other inputs seperated by spaces as normal
    // strings.
    // It also ignores whitespaces in values when being surrounded by quotation
    // marks, to enable inputs like this => regex:"kappa 123"
    static QRegularExpression predicateRegex(
        R"lit((?:(?<name>\w+):(?<value>".+?"|[^\s]+))|[^\s]+?(?=$|\s))lit");
    static QRegularExpression trimQuotationMarksRegex(R"(^"|"$)");

    QRegularExpressionMatchIterator it = predicateRegex.globalMatch(input);

    std::vector<std::unique_ptr<MessagePredicate>> predicates;
    QStringList authors;
    QStringList channels;

    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();

        QString name = match.captured("name");

        QString value = match.captured("value");
        value.remove(trimQuotationMarksRegex);

        // match predicates
        if (name == "from")
        {
            authors.append(value);
        }
        else if (name == "has" && value == "link")
        {
            predicates.push_back(std::make_unique<LinkPredicate>());
        }
        else if (name == "in")
        {
            channels.append(value);
        }
        else if (name == "is")
        {
            predicates.push_back(
                std::make_unique<MessageFlagsPredicate>(value));
        }
        else if (name == "regex")
        {
            predicates.push_back(std::make_unique<RegexPredicate>(value));
        }
        else
        {
            predicates.push_back(
                std::make_unique<SubstringPredicate>(match.captured()));
        }
    }

    if (!authors.empty())
        predicates.push_back(std::make_unique<AuthorPredicate>(authors));

    if (!channels.empty())
        predicates.push_back(std::make_unique<ChannelPredicate>(channels));

    return predicates;
}

}  // namespace chatterino
