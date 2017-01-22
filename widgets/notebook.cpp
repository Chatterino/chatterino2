#include "widgets/notebook.h"
#include "colorscheme.h"
#include "widgets/notebookbutton.h"
#include "widgets/notebookpage.h"
#include "widgets/notebooktab.h"
#include "widgets/settingsdialog.h"

#include <QFormLayout>
#include <QLayout>
#include <QList>
#include <QWidget>

namespace chatterino {
namespace widgets {

Notebook::Notebook(QWidget *parent)
    : QWidget(parent)
    , addButton(this)
    , settingsButton(this)
    , userButton(this)
    , selectedPage(nullptr)
{
    connect(&this->settingsButton, SIGNAL(clicked()), this,
            SLOT(settingsButtonClicked()));
    connect(&this->userButton, SIGNAL(clicked()), this,
            SLOT(usersButtonClicked()));
    connect(&this->addButton, SIGNAL(clicked()), this,
            SLOT(addPageButtonClicked()));

    this->settingsButton.resize(24, 24);
    this->settingsButton.icon = NotebookButton::IconSettings;

    this->userButton.resize(24, 24);
    this->userButton.move(24, 0);
    this->userButton.icon = NotebookButton::IconUser;

    this->addButton.resize(24, 24);

    this->addPage();
}

NotebookPage *
Notebook::addPage(bool select)
{
    auto tab = new NotebookTab(this);
    auto page = new NotebookPage(this, tab);

    tab->show();

    if (select || this->pages.count() == 0) {
        this->select(page);
    }

    this->pages.append(page);

    this->performLayout();

    return page;
}

void
Notebook::removePage(NotebookPage *page)
{
    int index = this->pages.indexOf(page);

    if (pages.size() == 1) {
        this->select(NULL);
    } else if (index == pages.count() - 1) {
        this->select(pages[index - 1]);
    } else {
        this->select(pages[index + 1]);
    }

    delete page->tab;
    delete page;

    this->pages.removeOne(page);

    if (this->pages.size() == 0) {
        addPage();
    }

    performLayout();
}

void
Notebook::select(NotebookPage *page)
{
    if (page == this->selectedPage)
        return;

    if (page != nullptr) {
        page->setHidden(false);
        page->tab->setSelected(true);
    }

    if (this->selectedPage != nullptr) {
        this->selectedPage->setHidden(true);
        this->selectedPage->tab->setSelected(false);
    }

    this->selectedPage = page;

    performLayout();
}

NotebookPage *
Notebook::tabAt(QPoint point, int &index)
{
    int i = 0;

    for (auto *page : pages) {
        if (page->tab->geometry().contains(point)) {
            index = i;
            return page;
        }

        i++;
    }

    index = -1;
    return nullptr;
}

void
Notebook::rearrangePage(NotebookPage *page, int index)
{
    int i1 = pages.indexOf(page);

    pages.move(pages.indexOf(page), index);

    int i2 = pages.indexOf(page);

    performLayout();
}

void
Notebook::performLayout()
{
    int x = 48, y = 0;
    int tabHeight = 16;
    bool first = true;

    for (auto &i : this->pages) {
        tabHeight = i->tab->height();

        if (!first &&
            (i == this->pages.last() ? tabHeight : 0) + x + i->tab->width() >
                width()) {
            y += i->tab->height();
            i->tab->moveAnimated(QPoint(0, y));
            x = i->tab->width();
        } else {
            i->tab->moveAnimated(QPoint(x, y));
            x += i->tab->width();
        }

        first = false;
    }

    this->addButton.move(x, y);

    if (this->selectedPage != nullptr) {
        this->selectedPage->move(0, y + tabHeight);
        this->selectedPage->resize(width(), height() - y - tabHeight);
    }
}

void
Notebook::resizeEvent(QResizeEvent *)
{
    performLayout();
}

void
Notebook::settingsButtonClicked()
{
    SettingsDialog *a = new SettingsDialog();

    a->show();
}

void
Notebook::usersButtonClicked()
{
}

void
Notebook::addPageButtonClicked()
{
    addPage(true);
}
}
}
