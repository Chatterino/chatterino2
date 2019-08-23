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

    QStringList searchedNames;
    QStringList searchedContent;

    ChannelPtr channel(new Channel(this->channelName_, Channel::Type::None));

    // Parse all usernames that should be searched for
    for (QString &word : text.split(' ', QString::SkipEmptyParts))
    {
        if (word.startsWith('@'))
        {
            searchedNames << word.remove('@');
        }
        else
        {
            searchedContent << word;
        }
    }

    // Build the query (this will be searched for in messages by the selected users)
    QString query = searchedContent.join(' ');

    for (size_t i = 0; i < this->snapshot_.size(); i++)
    {
        MessagePtr message = this->snapshot_[i];

        if (text.isEmpty() ||
            message->searchText.contains(query, Qt::CaseInsensitive))
        {
            // If username-only search is enabled, at least one name must match.
            // If it is not enabled, we can add the message directly.
            if (searchedNames.size() == 0 ||
                searchedNames.contains(message->displayName,
                                       Qt::CaseInsensitive))
            {
                channel->addMessage(message);
            }
        }
    }

    this->channelView_->setChannel(channel);
}

}  // namespace chatterino
