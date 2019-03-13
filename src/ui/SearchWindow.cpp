#include "ui/SearchWindow.hpp"

#include <QLineEdit>
#include <QPushButton>

#include "ab/Column.hpp"
#include "ab/Row.hpp"
#include "ab/util/MakeWidget.hpp"
#include "messages/Message.hpp"
#include "ui/ChannelView.hpp"

namespace chatterino::ui
{
    SearchWindow::SearchWindow()
    {
        this->initLayout();
        this->resize(400, 600);
    }

    void SearchWindow::initLayout()
    {
        this->setCenterLayout(ab::makeLayout<ab::Column>({
            ab::makeLayout<ab::Row>({
                // Search input
                ab::makeWidget<QLineEdit>([&](auto w) {
                    this->searchInput_ = w;
                    QObject::connect(w, &QLineEdit::returnPressed, this,
                        &SearchWindow::performSearch);
                }),
                // Search button
                ab::makeWidget<QPushButton>([&](auto w) {
                    w->setText("Search");
                    QObject::connect(w, &QPushButton::clicked, this,
                        &SearchWindow::performSearch);
                }),
            }),
            // Messageview
            ab::makeWidget<ChannelView>(
                [&](auto w) { this->messageView_ = w; }),
        }));
    }

    void SearchWindow::setMessages(std::shared_ptr<MessageContainer> messages)
    {
        this->messages_ = messages;
        this->performSearch();
        // this->setWindowTitle(
        //    "Searching in " + channel->getName() + "s history");
    }

    void SearchWindow::performSearch()
    {
        if (!this->messages_)
            return;

        QString text = searchInput_->text();

        auto out = std::make_shared<MessageContainer>();

        for (auto&& message : *this->messages_)
        {
            if (text.isEmpty() ||
                message->searchText.indexOf(
                    this->searchInput_->text(), 0, Qt::CaseInsensitive) != -1)
            {
                out->append(message);
            }
        }

        this->messageView_->setContainer(out);
    }
}  // namespace chatterino::ui
