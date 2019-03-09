#include "SplitInput.hpp"

#include "ab/Column.hpp"
#include "ab/FlatButton.hpp"
#include "ab/Row.hpp"
#include "ab/util/MakeWidget.hpp"
#include "ui/SplitInput.ResizingTextEdit.hpp"
#include "util/Resources.hpp"

#include "Room.hpp"

#include <QLabel>

namespace chatterino::ui
{
    SplitInput::SplitInput()
    {
        this->setSizePolicy(
            QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));

        // main layout
        this->setLayout(ab::makeLayout<ab::Row>({
            ab::makeWidget<ResizingTextEdit>([&](auto x) {
                this->edit_ = x;
                QObject::connect(x, &ResizingTextEdit::textChanged, this,
                    [this]() { this->updateInputLength(); });
            }),
            ab::makeLayout<ab::Column>({
                ab::makeWidget<QLabel>([&](auto x) {
                    this->inputLength_ = x;
                    x->setObjectName("input-length");
                    x->setTextFormat(Qt::PlainText);
                    x->setAlignment(Qt::AlignCenter);
                }),
                ab::stretch(),
                ab::makeWidget<ab::FlatButton>([](auto x) {
                    x->setObjectName("emote-button");
                    x->setPixmap(resources().buttons.emote3);
                    x->setEnableMargin(false);
                }),
            }),
        }));

        // misc
        this->updateInputLength();
    }

    ResizingTextEdit* SplitInput::edit() const
    {
        return this->edit_;
    }

    void SplitInput::setRoom(Room* room)
    {
        this->room_ = room;
    }

    void SplitInput::updateInputLength()
    {
        if (this->edit_ && this->inputLength_)
        {
            // get input length from room (dependant on provider)
            auto length = this->room_ ? this->room_->calcInputLength(
                                            this->edit_->toPlainText())
                                      : 0;

            // set text to length or ""
            if (length == 0)
                this->inputLength_->setText("");
            else
                this->inputLength_->setText(QString::number(length));
        }
    }
}  // namespace chatterino::ui
