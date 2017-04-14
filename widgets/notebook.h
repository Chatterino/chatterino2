#ifndef NOTEBOOK_H
#define NOTEBOOK_H

#include "widgets/notebookbutton.h"
#include "widgets/notebookpage.h"
#include "widgets/notebooktab.h"

#include <QList>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>

namespace  chatterino {
namespace  widgets {

class Notebook : public QWidget
{
    Q_OBJECT

public:
    enum HighlightType { none, highlighted, newMessage };

    Notebook(QWidget *parent);

    NotebookPage *addPage(bool select = false);

    void removePage(NotebookPage *page);
    void select(NotebookPage *page);

    NotebookPage *getSelectedPage()
    {
        return _selectedPage;
    }

    void performLayout(bool animate = true);

    NotebookPage *tabAt(QPoint point, int &index);
    void rearrangePage(NotebookPage *page, int index);

protected:
    void resizeEvent(QResizeEvent *);

    void settingsButtonMouseReleased(QMouseEvent *event);

public slots:
    void settingsButtonClicked();
    void usersButtonClicked();
    void addPageButtonClicked();

private:
    QList<NotebookPage *> _pages;

    NotebookButton _addButton;
    NotebookButton _settingsButton;
    NotebookButton _userButton;

    NotebookPage *_selectedPage;

public:
    void load(const boost::property_tree::ptree &tree);
    void save(boost::property_tree::ptree &tree);
    void loadDefaults();
};

}  // namespace  widgets
}  // namespace  chatterino

#endif  // NOTEBOOK_H
