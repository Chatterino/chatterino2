#include "widgets/resizingtextedit.hpp"

ResizingTextEdit::ResizingTextEdit()
    : keyPressed()
{
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    sizePolicy.setVerticalPolicy(QSizePolicy::Preferred);
    this->setSizePolicy(sizePolicy);

    QObject::connect(this, &QTextEdit::textChanged, this, &QWidget::updateGeometry);
}

QSize ResizingTextEdit::sizeHint() const
{
    return QSize(this->width(), this->heightForWidth(this->width()));
}

bool ResizingTextEdit::hasHeightForWidth() const
{
    return true;
}

int ResizingTextEdit::heightForWidth(int) const
{
    auto margins = this->contentsMargins();

    return margins.top() + document()->size().height() + margins.bottom() + 5;
}


void ResizingTextEdit::keyPressEvent(QKeyEvent *event)
{
    event->ignore();

    keyPressed(event);

    if (!event->isAccepted()) {
        QTextEdit::keyPressEvent(event);
    }
}
