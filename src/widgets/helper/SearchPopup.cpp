#include "SearchPopup.hpp"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "common/Channel.hpp"
#include "messages/Message.hpp"
#include "widgets/helper/ChannelView.hpp"

namespace chatterino {
namespace {
    ChannelPtr filter(const QString &text, const QString &channelName,
                      const LimitedQueueSnapshot<MessagePtr> &snapshot)
    {
        ChannelPtr channel(new Channel(channelName, Channel::Type::None));

        for (size_t i = 0; i < snapshot.size(); i++)
        {
            MessagePtr message = snapshot[i];

            if (text.isEmpty() ||
                message->searchText.indexOf(text, 0, Qt::CaseInsensitive) != -1)
            {
                channel->addMessage(message);
            }
        }

        return channel;
    }
}  // namespace

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

}  // namespace chatterino
