#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/notebookbutton.hpp"
#include "widgets/notebookpage.hpp"
#include "widgets/notebooktab.hpp"

#include <QList>
#include <QWidget>
#include <boost/property_tree/ptree.hpp>

namespace chatterino {

class ChannelManager;
class CompletionManager;

namespace widgets {

class MainWindow;

class Notebook : public BaseWidget
{
    Q_OBJECT

public:
    enum HighlightType { none, highlighted, newMessage };

    explicit Notebook(ChannelManager &_channelManager, MainWindow *parent);

    NotebookPage *addPage(bool select = false);

    void removePage(NotebookPage *page);
    void select(NotebookPage *page);

    NotebookPage *getSelectedPage() const
    {
        return selectedPage;
    }

    void performLayout(bool animate = true);

    NotebookPage *tabAt(QPoint point, int &index);
    void rearrangePage(NotebookPage *page, int index);

    void nextTab();
    void previousTab();

protected:
    void resizeEvent(QResizeEvent *);

    void settingsButtonMouseReleased(QMouseEvent *event);

public slots:
    void settingsButtonClicked();
    void usersButtonClicked();
    void addPageButtonClicked();

public:
    ChannelManager &channelManager;
    CompletionManager &completionManager;

private:
    QList<NotebookPage *> pages;

    NotebookButton addButton;
    NotebookButton settingsButton;
    NotebookButton userButton;

    NotebookPage *selectedPage = nullptr;

public:
    void load(const boost::property_tree::ptree &tree);
    void save(boost::property_tree::ptree &tree);
    void loadDefaults();
};

}  // namespace widgets
}  // namespace chatterino
