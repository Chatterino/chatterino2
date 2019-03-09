#include "SelectChannelDialog.hpp"

#include "ab/BaseWindow.hpp"
#include "ab/Column.hpp"
#include "ab/Notebook.hpp"
#include "ab/Row.hpp"
#include "ab/util/MakeWidget.hpp"

#include "Application.hpp"
#include "Provider.hpp"

#include <QLabel>
#include <QPushButton>

namespace chatterino
{
    namespace
    {
        class Page : public QWidget
        {
        public:
            std::function<RoomPtr()> getData;
        };
    }  // namespace

    void SelectChannel::showDialog(Application& app, const Room& currentRoom)
    {
        if (this->dialog_)
        {
            this->dialog_->raise();
            return;
        }

        // create new dialog
        auto window = std::make_unique<ab::Dialog>();
        auto ptr = window.get();

        window->setWindowTitle("Select room to join");
        window->setAttribute(Qt::WA_DeleteOnClose);
        QObject::connect(ptr, &ab::BaseWindow::closing, ptr, [this, ptr]() {
            if (this->dialog_.get() == ptr)
                this->dialog_.release();
        });

        window->setCenterLayout(ab::makeLayout<ab::Column>({
            // Notebook containing providers
            ab::makeWidget<ab::Notebook>([&](auto x) {
                this->notebook_ = x;

                for (auto&& provider : app.providers())
                {
                    auto page = ab::makeWidget<Page>([&](Page* x) {
                        auto [layout, getData] =
                            provider->buildSelectChannelLayout(
                                currentRoom.serialize());

                        x->getData = std::move(getData);

                        // create the select channel layout based on the
                        // current room
                        x->setLayout(layout);
                    });

                    x->addTab(provider->title(), page);
                }
            }),
            // Buttons
            ab::makeLayout<ab::Row>({
                ab::stretch(),
                ab::makeWidget<QPushButton>([=](QPushButton* w) {
                    w->setText("Ok");

                    QObject::connect(
                        w, &QPushButton::clicked, ptr, [this, ptr]() {
                            ptr->close();

                            if (auto page = dynamic_cast<Page*>(
                                    this->notebook_->selectedPage()))
                                emit this->accepted(page->getData());
                            else
                                assert(false);
                        });
                }),
                ab::makeWidget<QPushButton>([=](QPushButton* w) {
                    w->setText("Cancel");
                    QObject::connect(w, &QPushButton::clicked, ptr,
                        [ptr]() { ptr->close(); });
                }),
            }),
        }));

        window->resize(400, 300);
        window->show();

        this->dialog_ = std::move(window);
    }

    void SelectChannel::closeDialog()
    {
        this->dialog_.release()->deleteLater();
    }
}  // namespace chatterino
