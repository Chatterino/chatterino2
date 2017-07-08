#pragma once

#include <QCompleter>
#include <QKeyEvent>
#include <QTextEdit>
#include <boost/signals2.hpp>

class ResizingTextEdit : public QTextEdit
{
public:
    ResizingTextEdit();

    QSize sizeHint() const override;

    bool hasHeightForWidth() const override;

    boost::signals2::signal<void(QKeyEvent *)> keyPressed;

    void setCompleter(QCompleter *c);
    QCompleter *getCompleter() const;

protected:
    int heightForWidth(int) const override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QCompleter *completer = nullptr;
    QString textUnderCursor() const;
    bool nextCompletion = false;

private slots:
    void insertCompletion(const QString &completion);
};
