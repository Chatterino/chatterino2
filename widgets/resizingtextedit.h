#ifndef RESIZINGTEXTEDIT_H
#define RESIZINGTEXTEDIT_H

#include <QKeyEvent>
#include <QTextEdit>
#include <boost/signals2.hpp>

class ResizingTextEdit : public QTextEdit
{
public:
    ResizingTextEdit()
        : keyPressed()
    {
        auto sizePolicy = this->sizePolicy();
        sizePolicy.setHeightForWidth(true);
        sizePolicy.setVerticalPolicy(QSizePolicy::Preferred);
        this->setSizePolicy(sizePolicy);

        QObject::connect(this, &QTextEdit::textChanged, this,
                         &QWidget::updateGeometry);
    }

    QSize
    sizeHint() const override
    {
        return QSize(this->width(), this->heightForWidth(this->width()));
    }

    bool
    hasHeightForWidth() const override
    {
        return true;
    }

    boost::signals2::signal<void(QKeyEvent *)> keyPressed;

protected:
    int
    heightForWidth(int) const override
    {
        auto margins = this->contentsMargins();

        return margins.top() + document()->size().height() + margins.bottom() +
               5;
    }

    void
    keyPressEvent(QKeyEvent *event)
    {
        event->ignore();

        keyPressed(event);

        if (!event->isAccepted()) {
            QTextEdit::keyPressEvent(event);
        }
    }
};

#endif  // RESIZINGTEXTEDIT_H
