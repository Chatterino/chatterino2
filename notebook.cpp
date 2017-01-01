#include "QWidget"
#include "QList"
#include "QLayout"
#include "notebook.h"
#include "notebooktab.h"
#include "notebookpage.h"
#include "notebookbutton.h"
#include "QFormLayout"
#include "colorscheme.h"
#include "dialog.h"

Notebook::Notebook(QWidget *parent)
    : QWidget(parent),
      addButton(this),
      settingsButton(this),
      userButton(this)
{
    connect(&settingsButton, SIGNAL(clicked()), this, SLOT(settingsButtonClicked()));

    settingsButton.resize(24, 24);
    settingsButton.icon = NotebookButton::IconSettings;
    userButton.resize(24, 24);
    userButton.move(24, 0);
    userButton.icon = NotebookButton::IconUser;
    addButton.resize(24, 24);
}

void Notebook::settingsButtonClicked()
{
    auto a = new Dialog();

    a->show();
}

NotebookPage* Notebook::addPage()
{
    auto tab = new NotebookTab(this);
    auto page = new NotebookPage(this, tab);

    if (pages.count() == 0)
    {
        select(page);
    }

    this->pages.append(page);

    return page;
}

void Notebook::select(NotebookPage* page)
{
    if (page == selected) return;

    if (page != nullptr)
    {
        page->setHidden(false);
        page->tab->setSelected(true);
    }

    if (selected != nullptr)
    {
        selected->setHidden(true);
        selected->tab->setSelected(false);
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
