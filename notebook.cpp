#include "QWidget"
#include "QList"
#include "QLayout"
#include "notebook.h"
#include "notebooktab.h"
#include "notebookpage.h"
#include "notebookbutton.h"
#include "QFormLayout"

Notebook::Notebook(QWidget *parent)
    : QWidget(parent),
      addButton(this),
      settingsButton(this),
      userButton(this)
{
    settingsButton.resize(24, 24);
    settingsButton.icon = NotebookButton::IconSettings;
    userButton.resize(24, 24);
    userButton.move(24, 0);
    userButton.icon = NotebookButton::IconUser;
    addButton.resize(24, 24);
}

NotebookPage* Notebook::addPage()
{
    auto tab = new NotebookTab(this);
    auto page = new NotebookPage(tab);

    if (pages.count() == 0)
    {
        select(page);
    }

    this->pages.append(page);

    return page;
}

void Notebook::select(NotebookPage* page)
{
    if (selected != nullptr)
    {
        selected->setParent(nullptr);
        selected->tab->setSelected(false);
    }

    if (page != nullptr)
    {
        page->setParent(this);
        page->tab->setSelected(true);
    }

    selected = page;

    performLayout();
}

void Notebook::performLayout()
{
    int x = 48, y = 0;
    int tabHeight = 16;
    bool first = true;

    for (auto &i : pages)
    {
        tabHeight = i->tab->height();

        if (!first && (i == pages.last() ? tabHeight : 0) + x + i->tab->width() > width())
        {
            y +=i->tab->height();
            i->tab->move(0, y);
            x = i->tab->width();
        }
        else
        {
            i->tab->move(x, y);
            x += i->tab->width();
        }

        first = false;
    }

    this->addButton.move(x, y);

    if (selected != nullptr)
    {
        selected->move(0, y + tabHeight);
        selected->resize(width(), height() - y - tabHeight);
    }
}

void Notebook::resizeEvent(QResizeEvent *)
{
    performLayout();
}
