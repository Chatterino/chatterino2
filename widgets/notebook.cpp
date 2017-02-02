#include "widgets/notebook.h"
#include "colorscheme.h"
#include "widgets/notebookbutton.h"
#include "widgets/notebookpage.h"
#include "widgets/notebooktab.h"
#include "widgets/settingsdialog.h"

#include <QDebug>
#include <QFile>
#include <QFormLayout>
#include <QLayout>
#include <QList>
#include <QStandardPaths>
#include <QWidget>
#include <boost/foreach.hpp>

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

    Settings::getInstance().hidePreferencesButton.valueChanged.connect(
        [this](const bool &) { this->performLayout(); });
    Settings::getInstance().hideUserButton.valueChanged.connect(
        [this](const bool &) { this->performLayout(); });
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
        page->tab->raise();
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
        if (page->tab->getDesiredRect().contains(point)) {
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
Notebook::performLayout(bool animated)
{
    int x = 0, y = 0;

    if (Settings::getInstance().hidePreferencesButton.get()) {
        settingsButton.hide();
    } else {
        settingsButton.show();
        x += 24;
    }
    if (Settings::getInstance().hideUserButton.get()) {
        userButton.hide();
    } else {
        userButton.show();
        x += 24;
    }

    int tabHeight = 16;
    bool first = true;

    for (auto &i : this->pages) {
        tabHeight = i->tab->height();

        if (!first &&
            (i == this->pages.last() ? tabHeight : 0) + x + i->tab->width() >
                width()) {
            y += i->tab->height();
            i->tab->moveAnimated(QPoint(0, y), animated);
            x = i->tab->width();
        } else {
            i->tab->moveAnimated(QPoint(x, y), animated);
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
    performLayout(false);
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

void
Notebook::load(const boost::property_tree::ptree &tree)
{
    // Read a list of tabs
    try {
        BOOST_FOREACH (const boost::property_tree::ptree::value_type &v,
                       tree.get_child("tabs.")) {
            bool select = v.second.get<bool>("selected", false);

            auto page = this->addPage(select);
            auto tab = page->tab;
            tab->load(v.second);
            page->load(v.second);
        }
    } catch (boost::property_tree::ptree_error &) {
        // can't read tabs
    }

    if (this->pages.size() == 0) {
        // No pages saved, show default stuff
        this->loadDefaults();
    }
}

void
Notebook::save(boost::property_tree::ptree &tree)
{
    boost::property_tree::ptree tabs;

    // Iterate through all tabs and add them to our tabs property thing
    for (const auto &page : this->pages) {
        boost::property_tree::ptree pTab = page->tab->save();

        boost::property_tree::ptree pChats = page->save();

        if (pChats.size() > 0) {
            pTab.add_child("columns", pChats);
        }

        tabs.push_back(std::make_pair("", pTab));
    }

    tree.add_child("tabs", tabs);
}

void
Notebook::loadDefaults()
{
    this->addPage();
}

}  // namespace widgets
}  // namespace chatterino
