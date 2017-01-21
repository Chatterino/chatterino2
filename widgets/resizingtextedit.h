#ifndef RESIZINGTEXTEDIT_H
#define RESIZINGTEXTEDIT_H

#include <QTextEdit>

class ResizingTextEdit : public QTextEdit
{
public:
    ResizingTextEdit()
    {
        auto sizePolicy = this->sizePolicy();
        sizePolicy.setHeightForWidth(true);
        sizePolicy.setVerticalPolicy(QSizePolicy::Preferred);
        this->setSizePolicy(sizePolicy);

        QObject::connect(this, &QTextEdit::textChanged, this, &updateGeometry);
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

protected:
    int
    heightForWidth(int w) const override
    {
        auto margins = this->contentsMargins();

        int documentWidth;

        if (w >= margins.left() + margins.right()) {
            documentWidth = w - margins.left() - margins.right();
        } else {
            documentWidth = 0;
        }

        auto document = this->document()->clone();

        return margins.top() + document->size().height() + margins.bottom() + 2;
    }
};

#endif  // RESIZINGTEXTEDIT_H
