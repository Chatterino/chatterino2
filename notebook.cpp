#include "QWidget"
#include "QList"
#include "notebook.h"
#include "notebooktab.h"
#include "notebookpage.h"
#include "notebookbutton.h"

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
    auto page = new NotebookPage(this, tab);

    this->pages.append(page);

    layout();

    return page;
}

void Notebook::layout()
{
    int x = 48, y = 0;
    int tabHeight = 16;
    bool firstInLine = true;

    for (auto &i : this->pages)
    {
        tabHeight = i->tab->height();

        if (!firstInLine && (i == this->pages.last() ? tabHeight : 0) + x + i->width() > this->width())
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

        firstInLine = false;
    }

    this->addButton.move(x, y);
}

void Notebook::resizeEvent(QResizeEvent *)
{
    layout();
}
