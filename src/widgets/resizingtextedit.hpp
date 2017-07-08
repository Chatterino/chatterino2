#pragma once

#include <QKeyEvent>
#include <QTextEdit>
#include <QCompleter>
#include <boost/signals2.hpp>

class ResizingTextEdit : public QTextEdit
{
public:
    ResizingTextEdit();

    QSize sizeHint() const override;

    bool hasHeightForWidth() const override;

    boost::signals2::signal<void(QKeyEvent *)> keyPressed;

protected:
    int heightForWidth(int) const override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QCompleter completer;
};
