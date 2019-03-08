#include "SearchPopup.hpp"

#if 0
#    include <QHBoxLayout>
#    include <QLineEdit>
#    include <QPushButton>
#    include <QVBoxLayout>

#    include "common/Channel.hpp"
#    include "messages/Message.hpp"
#    include "widgets/helper/ChannelView.hpp"

namespace chatterino
{
    SearchPopup::SearchPopup()
    {
        this->initLayout();
        this->resize(400, 600);
    }

    void SearchPopup::initLayout()
    {
        // VBOX
        {
            QVBoxLayout* layout1 = new QVBoxLayout(this);
            layout1->setMargin(0);

            // HBOX
            {
                QHBoxLayout* layout2 = new QHBoxLayout(this);
                layout2->setMargin(6);

                // SEARCH INPUT
                {
                    this->searchInput_ = new QLineEdit(this);
                    layout2->addWidget(this->searchInput_);
                    QObject::connect(this->searchInput_,
                        &QLineEdit::returnPressed,
                        [this] { this->performSearch(); });
                }

                // SEARCH BUTTON
                {
                    QPushButton* searchButton = new QPushButton(this);
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

    void SearchPopup::setChannel(ChannelPtr channel)
    {
        this->snapshot_ = channel->getMessageSnapshot();
        this->performSearch();

        this->setWindowTitle(
            "Searching in " + channel->getName() + "s history");
    }

    void SearchPopup::performSearch()
    {
        QString text = searchInput_->text();

        ChannelPtr channel(new Channel("search", Channel::Type::None));

        for (size_t i = 0; i < this->snapshot_.size(); i++)
        {
            MessagePtr message = this->snapshot_[i];

            if (text.isEmpty() ||
                message->searchText.indexOf(
                    this->searchInput_->text(), 0, Qt::CaseInsensitive) != -1)
            {
                channel->addMessage(message);
            }
        }

        this->channelView_->setChannel(channel);
    }

}  // namespace chatterino
#endif
