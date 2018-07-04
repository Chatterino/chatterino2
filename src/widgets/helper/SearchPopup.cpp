#include "SearchPopup.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "common/Channel.hpp"
#include "widgets/helper/ChannelView.hpp"

namespace chatterino {

SearchPopup::SearchPopup()
{
    this->initLayout();
    this->resize(400, 600);
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
                this->searchInput = new QLineEdit(this);
                layout2->addWidget(this->searchInput);
                QObject::connect(this->searchInput, &QLineEdit::returnPressed,
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
            this->channelView = new ChannelView(this);

            layout1->addWidget(this->channelView);
        }

        this->setLayout(layout1);
    }
}

void SearchPopup::setChannel(ChannelPtr channel)
{
    this->snapshot = channel->getMessageSnapshot();
    this->performSearch();

    this->setWindowTitle("Searching in " + channel->name + "s history");
}

void SearchPopup::performSearch()
{
    QString text = searchInput->text();

    ChannelPtr channel(new Channel("search", Channel::None));

    for (size_t i = 0; i < this->snapshot.getLength(); i++) {
        MessagePtr message = this->snapshot[i];

        if (text.isEmpty() ||
            message->searchText.indexOf(this->searchInput->text(), 0, Qt::CaseInsensitive) != -1) {
            channel->addMessage(message);
        }
    }

    this->channelView->setChannel(channel);
}

}  // namespace chatterino
