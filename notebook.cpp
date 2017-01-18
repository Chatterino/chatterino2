#include "notebook.h"
#include "colorscheme.h"
#include "dialog.h"
#include "notebookbutton.h"
#include "notebookpage.h"
#include "notebooktab.h"
#include "settingsdialog.h"

#include <QFormLayout>
#include <QLayout>
#include <QList>
#include <QWidget>

Notebook::Notebook(QWidget *parent)
    : QWidget(parent)
    , addButton(this)
    , settingsButton(this)
    , userButton(this)
{
    connect(&this->settingsButton, SIGNAL(clicked()), this,
            SLOT(settingsButtonClicked()));

    this->settingsButton.resize(24, 24);
    this->settingsButton.icon = NotebookButton::IconSettings;
    this->userButton.resize(24, 24);
    this->userButton.move(24, 0);
    this->userButton.icon = NotebookButton::IconUser;
    this->addButton.resize(24, 24);
}

void
Notebook::settingsButtonClicked()
{
    SettingsDialog *a = new SettingsDialog();

    a->show();
}

NotebookPage *
Notebook::addPage()
{
    auto tab = new NotebookTab(this);
    auto page = new NotebookPage(this, tab);

    if (this->pages.count() == 0) {
        select(page);
    }

    this->pages.append(page);

    return page;
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
