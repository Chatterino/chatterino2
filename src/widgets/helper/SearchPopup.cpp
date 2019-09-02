#include "SearchPopup.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "widgets/helper/ChannelView.hpp"

namespace chatterino {

SearchPopup::SearchPopup()
{
    this->initLayout();
    this->resize(400, 600);
}

void SearchPopup::setChannel(ChannelPtr channel)
{
    this->channelName_ = channel->getName();
    this->snapshot_ = channel->getMessageSnapshot();
    this->performSearch();

    this->setWindowTitle("Searching in " + channel->getName() + "s history");
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

        // HBOX
        {
            QHBoxLayout *layout2 = new QHBoxLayout(this);
            layout2->setMargin(6);

            // SEARCH INPUT
            {
                this->searchInput_ = new QLineEdit(this);
                layout2->addWidget(this->searchInput_);
                QObject::connect(this->searchInput_, &QLineEdit::returnPressed,
                                 [this] { this->performSearch(); });
            }

            // SEARCH BUTTON
            {
                QPushButton *searchButton = new QPushButton(this);
                searchButton->setText("Search");
                layout2->addWidget(searchButton);
                QObject::connect(searchButton, &QPushButton::clicked,
                                 [this] { this->performSearch(); });
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

void SearchPopup::performSearch()
{
    QString text = searchInput_->text();
    QStringList searchedNames = parseSearchedUsers(text);

    // TODO: Implement this properly
    // Remove all filter tags
    for (QString &word : text.split(' ', QString::SkipEmptyParts))
    {
        if (word.startsWith("from:", Qt::CaseInsensitive))
            text.remove(word);
    }

    ChannelPtr channel(new Channel(this->channelName_, Channel::Type::None));

    // TODO: Remove
    for (auto name : searchedNames)
        std::cout << "Parsed user name: " << name.toStdString() << std::endl;
    
    // TODO: Remove
    std::cout << "After Parsing: Searching for " << text.trimmed().toStdString() << std::endl;

    for (size_t i = 0; i < this->snapshot_.size(); i++)
    {
        MessagePtr message = this->snapshot_[i];

        if (text.isEmpty() || message->searchText.contains(text.trimmed(), Qt::CaseInsensitive))
        {
            if (searchedNames.size() == 0 || searchedNames.contains(message->displayName, Qt::CaseInsensitive))
            {
                channel->addMessage(message);
            }
        }
    }

    this->channelView_->setChannel(channel);
}

QStringList SearchPopup::parseSearchedUsers(const QString& input)
{
    QStringList parsedUserNames;

    for (QString &word : input.split(' ', QString::SkipEmptyParts))
    {
        // Users can either be searched for by specifying them comma-seperated:
        // "from:user1,user2,user3"
        // or by supplying multiple "from" tags:
        // "from:user1 from:user2 from:user3"

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

        // The "from" tag shouldn't be part of the actual search query so we
        // remove it from "input"
        // input.remove(word);
    }

    return parsedUserNames;
}

}  // namespace chatterino
