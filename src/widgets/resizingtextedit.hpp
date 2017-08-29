#pragma once

#include "fontmanager.hpp"

#include <QFont>
#include <QCompleter>
#include <QKeyEvent>
#include <QTextEdit>
#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

class ResizingTextEdit : public QTextEdit
{
public:
    ResizingTextEdit();

    QFont &getFont() const;

    QSize sizeHint() const override;

    bool hasHeightForWidth() const override;

    boost::signals2::signal<void(QKeyEvent *)> keyPressed;

    void setCompleter(QCompleter *c);
    QCompleter *getCompleter() const;

protected:
    virtual int heightForWidth(int) const override;
    virtual void keyPressEvent(QKeyEvent *event) override;

private:
    QCompleter *completer = nullptr;

    FontManager::Type _font = FontManager::Small;

    // hadSpace is set to true in case the "textUnderCursor" word was after a space
    QString textUnderCursor(bool *hadSpace = nullptr) const;

    bool nextCompletion = false;

private slots:
    void insertCompletion(const QString &completion);
};

}  // namespace widgets
}  // namespace chatterino
