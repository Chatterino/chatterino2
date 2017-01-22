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
}

NotebookPage *
Notebook::addPage(bool select)
{
    auto tab = new NotebookTab(this);
    auto page = new NotebookPage(this, tab);

    if (select || this->pages.count() == 0) {
        this->select(page);
    }

    this->pages.append(page);

    performLayout();

    return page;
}

void
Notebook::removePage(NotebookPage *page)
{
    int index = pages.indexOf(page);

    if (pages.size() == 1) {
        select(NULL);
    } else if (index == pages.count() - 1) {
        select(pages[index - 1]);
    } else {
        select(pages[index + 1]);
    }

    delete page->tab;
    delete page;

    pages.removeOne(page);

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
            i->tab->move(0, y);
            x = i->tab->width();
        } else {
            i->tab->move(x, y);
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
