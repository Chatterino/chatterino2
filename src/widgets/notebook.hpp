#pragma once

#include "widgets/basewidget.hpp"
#include "widgets/helper/notebookbutton.hpp"
#include "widgets/helper/notebooktab.hpp"
#include "widgets/splitcontainer.hpp"

#include <QList>
#include <QWidget>

namespace chatterino {

class ChannelManager;

namespace widgets {

class Window;

class Notebook : public BaseWidget
{
    Q_OBJECT

    std::string settingRoot;

public:
    enum HighlightType { none, highlighted, newMessage };

    explicit Notebook(ChannelManager &_channelManager, Window *parent, bool _showButtons,
                      const std::string &settingPrefix);

    SplitContainer *addNewPage();
    SplitContainer *addPage(const std::string &uuid, bool select = false);

    void removePage(SplitContainer *page);
    void select(SplitContainer *page);

    SplitContainer *getSelectedPage() const
    {
        return selectedPage;
    }

    void performLayout(bool animate = true);

    int tabCount();
    SplitContainer *tabAt(QPoint point, int &index);
    void rearrangePage(SplitContainer *page, int index);

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

private:
    QList<SplitContainer *> pages;

    NotebookButton addButton;
    NotebookButton settingsButton;
    NotebookButton userButton;

    SplitContainer *selectedPage = nullptr;

    bool showButtons;

    void loadContainers();

public:
    void save();
};

}  // namespace widgets
}  // namespace chatterino
