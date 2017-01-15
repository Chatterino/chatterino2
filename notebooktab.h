#ifndef NOTEBOOKTAB_H
#define NOTEBOOKTAB_H

#include <QWidget>

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

    NotebookTab(Notebook *m_notebook);

    void calcSize();

    NotebookPage *page;

    const QString &
    text() const
    {
        return m_text;
    }

    void
    setText(const QString &text)
    {
        m_text = text;
    }

    bool
    selected()
    {
        return m_selected;
    }

    void
    setSelected(bool value)
    {
        m_selected = value;
        repaint();
    }

    HighlightStyle
    highlightStyle() const
    {
        return m_highlightStyle;
    }

    void
    setHighlightStyle(HighlightStyle style)
    {
        m_highlightStyle = style;
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
    Notebook *m_notebook;

    QString m_text;

    bool m_selected;
    bool m_mouseOver;
    bool m_mouseDown;
    HighlightStyle m_highlightStyle;
};

#endif  // NOTEBOOKTAB_H
