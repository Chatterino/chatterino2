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
    ~NotebookTab();

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
    void paintEvent(QPaintEvent *) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

private:
    Notebook *notebook;

    QString text;

    bool selected;
    bool mouseOver;
    bool mouseDown;
    bool mouseOverX;
    bool mouseDownX;

    HighlightStyle highlightStyle;

    QRect
    getXRect()
    {
        return QRect(this->width() - 20, 4, 16, 16);
    }

private slots:
    void
    hideTabXChanged(bool value)
    {
        calcSize();
        repaint();
    }
};
}
}

#endif  // NOTEBOOKTAB_H
