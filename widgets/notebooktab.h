#ifndef NOTEBOOKTAB_H
#define NOTEBOOKTAB_H

#include <QWidget>

namespace chatterino {
namespace widgets {

class Notebook;
class NotebookPage;

class NotebookTab : public QWidget
{
    Q_OBJECT

public:
    enum HighlightStyle {
        HighlightNone,
        HighlightHighlighted,
        HighlightNewMessage
    };

    NotebookTab(Notebook *notebook);

    void calcSize();

    NotebookPage *page;

    const QString &
    getText() const
    {
        return this->text;
    }

    void
    setText(const QString &text)
    {
        this->text = text;
    }

    bool
    getSelected()
    {
        return this->selected;
    }

    void
    setSelected(bool value)
    {
        this->selected = value;
        repaint();
    }

    HighlightStyle
    getHighlightStyle() const
    {
        return this->highlightStyle;
    }

    void
    setHighlightStyle(HighlightStyle style)
    {
        this->highlightStyle = style;
        repaint();
    }

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

    void mousePressEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *) Q_DECL_OVERRIDE;
    void enterEvent(QEvent *) Q_DECL_OVERRIDE;
    void leaveEvent(QEvent *) Q_DECL_OVERRIDE;

    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;

private:
    Notebook *notebook;

    QString text;

    bool selected;
    bool mouseOver;
    bool mouseDown;
    HighlightStyle highlightStyle;
};
}
}

#endif  // NOTEBOOKTAB_H
