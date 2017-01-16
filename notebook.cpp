#include "notebook.h"
#include "QFormLayout"
#include "QLayout"
#include "QList"
#include "QWidget"
#include "colorscheme.h"
#include "dialog.h"
#include "notebookbutton.h"
#include "notebookpage.h"
#include "notebooktab.h"
#include "settingsdialog.h"

Notebook::Notebook(QWidget *parent)
    : QWidget(parent)
    , m_addButton(this)
    , m_settingsButton(this)
    , m_userButton(this)
{
    connect(&m_settingsButton, SIGNAL(clicked()), this,
            SLOT(settingsButtonClicked()));

    m_settingsButton.resize(24, 24);
    m_settingsButton.icon = NotebookButton::IconSettings;
    m_userButton.resize(24, 24);
    m_userButton.move(24, 0);
    m_userButton.icon = NotebookButton::IconUser;
    m_addButton.resize(24, 24);
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

    if (m_pages.count() == 0) {
        select(page);
    }

    this->m_pages.append(page);

    return page;
}

void
Notebook::select(NotebookPage *page)
{
    if (page == m_selected)
        return;

    if (page != nullptr) {
        page->setHidden(false);
        page->tab->setSelected(true);
    }

    if (m_selected != nullptr) {
        m_selected->setHidden(true);
        m_selected->tab->setSelected(false);
    }

    m_selected = page;

    performLayout();
}

void
Notebook::performLayout()
{
    int x = 48, y = 0;
    int tabHeight = 16;
    bool first = true;

    for (auto &i : m_pages) {
        tabHeight = i->tab->height();

        if (!first &&
            (i == m_pages.last() ? tabHeight : 0) + x + i->tab->width() >
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

    this->m_addButton.move(x, y);

    if (m_selected != nullptr) {
        m_selected->move(0, y + tabHeight);
        m_selected->resize(width(), height() - y - tabHeight);
    }
}

void
Notebook::resizeEvent(QResizeEvent *)
{
    performLayout();
}
