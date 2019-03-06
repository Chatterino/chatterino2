#include "Split.hpp"

#include <QLabel>

#include "ab/BaseWidget.hpp"
#include "ab/Column.hpp"
#include "ab/Row.hpp"
#include "ab/util/FunctionEventFilter.hpp"
#include "ab/util/MakeWidget.hpp"

#include "Application.hpp"
#include "Room.hpp"
#include "ui/SplitHeader.hpp"
#include "ui/SplitInput.ResizingTextEdit.hpp"
#include "ui/SplitInput.hpp"

#include "dialogs/SelectChannelDialog.hpp"

#include <QJsonObject>
#include <QJsonValue>
#include "Provider.hpp"

namespace chatterino::ui
{
    Split::Split(Application& app)
        : app_(app)
    {
        this->initLayout();

        // this->setRoom(emptyRoom());
        this->setRoom(
            app.providers().at(0)->addRoom({{"channel", "fourtf_xd"}}));

        QObject::connect(&this->selectChannel_, &SelectChannel::accepted, this,
            [this](auto&& room) { this->setRoom(room); });
    }

    void Split::initLayout()
    {
        this->setLayout(ab::makeLayout<ab::Column>({
            ab::makeWidget<SplitHeader>([&](auto x) {
                this->header_ = x;
                x->installEventFilter(new ab::FunctionEventFilter(
                    this, [this](auto, QEvent* event) {
                        if (event->type() == QEvent::MouseButtonDblClick &&
                            static_cast<QMouseEvent*>(event)->button() ==
                                Qt::LeftButton)
                        {
                            this->showSelectDialog();
                        }
                        return false;
                    }));
            }),
            ab::makeLayout<ab::Column>(
                [&](auto x) { this->contentLayout_ = x; }, {}),
            ab::makeWidget<SplitInput>([&](auto x) {
                this->input_ = x;

                QObject::connect(
                    x->edit(), &ResizingTextEdit::focused, this, [this]() {
                        ab::setPropertyAndPolish(this, "selected", true);
                        ab::polishChildren(this);
                    });
                QObject::connect(
                    x->edit(), &ResizingTextEdit::unfocused, this, [this]() {
                        ab::setPropertyAndPolish(this, "selected", QVariant());
                        ab::polishChildren(this);
                    });
            }),
        }));
    }

    void Split::setRoom(const RoomPtr& room)
    {
        this->room_ = room;

        // delete old roomView
        if (this->roomView_)
        {
            this->contentLayout_->removeWidget(this->roomView_);
            this->roomView_->deleteLater();
        }

        // set the room of all child widgets
        if (this->header_)
            this->header_->setRoom(room.get());
        if (this->input_)
            this->input_->setRoom(room.get());

        // set content
        if (this->contentLayout_)
        {
            if (this->contentLayout_->count())
                delete this->contentLayout_->takeAt(0);

            auto view = room->createView(this);
            view->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
            this->roomView_ = view;
            this->contentLayout_->addWidget(view);
        }
    }

    void Split::showSelectDialog()
    {
        this->selectChannel_.showDialog(this->app_, *this->room_);
    }

    // EVENTS
    void Split::mousePressEvent(QMouseEvent* e)
    {
        if (this->input_)
            this->input_->edit()->setFocus();

        ab::BaseWidget::mousePressEvent(e);
    }
}  // namespace chatterino::ui
