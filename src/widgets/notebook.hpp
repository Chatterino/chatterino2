#pragma once

#include "widgets/notebookbutton.hpp"
#include "widgets/notebookpage.hpp"
#include "widgets/notebooktab.hpp"

#include <QList>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>

namespace chatterino {

class ChannelManager;

namespace widgets {

class Notebook : public QWidget
{
    Q_OBJECT

public:
    enum HighlightType { none, highlighted, newMessage };

    Notebook(ChannelManager &_channelManager, QWidget *parent);

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
    ChannelManager &channelManager;

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

}  // namespace widgets
}  // namespace chatterino
