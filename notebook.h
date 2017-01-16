#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include <QList>
#include <QWidget>
#include "notebookbutton.h"
#include "notebookpage.h"
#include "notebooktab.h"

class Notebook : public QWidget
{
    Q_OBJECT

public:
    Notebook(QWidget *parent);

    NotebookPage *addPage();

    enum HighlightType { none, highlighted, newMessage };

    void select(NotebookPage *page);

    NotebookPage *
    selected()
    {
        return m_selected;
    }

    void performLayout();

protected:
    void resizeEvent(QResizeEvent *);

    void settingsButtonMouseReleased(QMouseEvent *event);

public slots:
    void settingsButtonClicked();

private:
    QList<NotebookPage *> m_pages;

    NotebookButton m_addButton;
    NotebookButton m_settingsButton;
    NotebookButton m_userButton;

    NotebookPage *m_selected = nullptr;
};

#endif  // NOTEBOOK_H
