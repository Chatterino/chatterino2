#include "SplitHeader.hpp"

#include <QLabel>

#include "Room.hpp"
#include "ab/FlatButton.hpp"
#include "ab/Row.hpp"
#include "ab/util/MakeWidget.hpp"
#include "ui/Dropdown.hpp"
#include "util/Resources.hpp"

namespace chatterino::ui
{
    SplitHeader::SplitHeader()
    {
        this->setLayout(ab::makeLayout<ab::Row>({
            ab::makeWidget<QLabel>([&](auto x) {
                this->label_ = x;
                this->label_->setTextFormat(Qt::PlainText);
                this->label_->setAlignment(Qt::AlignCenter);
            }),
            ab::makeWidget<ab::FlatButton>([&](ab::FlatButton* x) {
                x->setDim(true);

                // TODO: add dark version
                x->setPixmap(resources().buttons.menuLight);
                x->setRipple(false);

                QObject::connect(
                    x, &ab::FlatButton::leftMousePress, this, [this]() {
                        // TODO: sometimes deletes menu before this check
                        if (this->menu_)
                            return;

                        Dropdown dropdown(this);
                        this->room_->fillDropdown(dropdown);

                        auto w = dropdown.releaseWidget();
                        this->menu_ = w;
                        w->updateGeometry();
                        w->show();
                        w->move(this->mapToGlobal({
                            this->width() - w->width(),
                            this->height(),
                        }));
                    });
            }),
        }));

        this->setRoom(nullptr);
    }

    void SplitHeader::setRoom(Room* value)
    {
        // use empty room if null
        if (!value)
            value = emptyRoom().get();

        this->room_ = value;

        // connect to state change signals
        this->cowner_.clear();

        this->cowner_ += QObject::connect(
            value, &Room::titleChanged, this->label_, &QLabel::setText);

        // update state
        if (this->label_)
            this->label_->setText(value->title());
    }
}  // namespace chatterino::ui
