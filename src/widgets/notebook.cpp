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
#include <QShortcut>
#include <QStandardPaths>
#include <QWidget>
#include <boost/foreach.hpp>

namespace chatterino {
namespace widgets {

Notebook::Notebook(QWidget *parent)
    : QWidget(parent)
    , _addButton(this)
    , _settingsButton(this)
    , _userButton(this)
    , _selectedPage(nullptr)
{
    connect(&_settingsButton, SIGNAL(clicked()), this, SLOT(settingsButtonClicked()));
    connect(&_userButton, SIGNAL(clicked()), this, SLOT(usersButtonClicked()));
    connect(&_addButton, SIGNAL(clicked()), this, SLOT(addPageButtonClicked()));

    _settingsButton.resize(24, 24);
    _settingsButton.icon = NotebookButton::IconSettings;

    _userButton.resize(24, 24);
    _userButton.move(24, 0);
    _userButton.icon = NotebookButton::IconUser;

    _addButton.resize(24, 24);

    SettingsManager::getInstance().hidePreferencesButton.valueChanged.connect(
        [this](const bool &) { performLayout(); });
    SettingsManager::getInstance().hideUserButton.valueChanged.connect(
        [this](const bool &) { performLayout(); });
}

NotebookPage *Notebook::addPage(bool select)
{
    auto tab = new NotebookTab(this);
    auto page = new NotebookPage(this, tab);

    tab->show();

    if (select || _pages.count() == 0) {
        this->select(page);
    }

    _pages.append(page);

    performLayout();

    return page;
}

void Notebook::removePage(NotebookPage *page)
{
    int index = _pages.indexOf(page);

    if (_pages.size() == 1) {
        select(NULL);
    } else if (index == _pages.count() - 1) {
        select(_pages[index - 1]);
    } else {
        select(_pages[index + 1]);
    }

    delete page->getTab();
    delete page;

    _pages.removeOne(page);

    if (_pages.size() == 0) {
        addPage();
    }

    performLayout();
}

void Notebook::select(NotebookPage *page)
{
    if (page == _selectedPage)
        return;

    if (page != nullptr) {
        page->setHidden(false);
        page->getTab()->setSelected(true);
        page->getTab()->raise();
    }

    if (_selectedPage != nullptr) {
        _selectedPage->setHidden(true);
        _selectedPage->getTab()->setSelected(false);
    }

    _selectedPage = page;

    performLayout();
}

NotebookPage *Notebook::tabAt(QPoint point, int &index)
{
    int i = 0;

    for (auto *page : _pages) {
        if (page->getTab()->getDesiredRect().contains(point)) {
            index = i;
            return page;
        }

        i++;
    }

    index = -1;
    return nullptr;
}

void Notebook::rearrangePage(NotebookPage *page, int index)
{
    _pages.move(_pages.indexOf(page), index);

    performLayout();
}

void Notebook::performLayout(bool animated)
{
    int x = 0, y = 0;

    if (SettingsManager::getInstance().hidePreferencesButton.get()) {
        _settingsButton.hide();
    } else {
        _settingsButton.show();
        x += 24;
    }
    if (SettingsManager::getInstance().hideUserButton.get()) {
        _userButton.hide();
    } else {
        _userButton.move(x, 0);
        _userButton.show();
        x += 24;
    }

    int tabHeight = 16;
    bool first = true;

    for (auto &i : _pages) {
        tabHeight = i->getTab()->height();

        if (!first && (i == _pages.last() ? tabHeight : 0) + x + i->getTab()->width() > width()) {
            y += i->getTab()->height();
            i->getTab()->moveAnimated(QPoint(0, y), animated);
            x = i->getTab()->width();
        } else {
            i->getTab()->moveAnimated(QPoint(x, y), animated);
            x += i->getTab()->width();
        }

        first = false;
    }

    _addButton.move(x, y);

    if (_selectedPage != nullptr) {
        _selectedPage->move(0, y + tabHeight);
        _selectedPage->resize(width(), height() - y - tabHeight);
    }
}

void Notebook::resizeEvent(QResizeEvent *)
{
    performLayout(false);
}

void Notebook::settingsButtonClicked()
{
    SettingsDialog::showDialog();
}

void Notebook::usersButtonClicked()
{
}

void Notebook::addPageButtonClicked()
{
    addPage(true);
}

void Notebook::load(const boost::property_tree::ptree &tree)
{
    // Read a list of tabs
    try {
        BOOST_FOREACH (const boost::property_tree::ptree::value_type &v, tree.get_child("tabs.")) {
            bool select = v.second.get<bool>("selected", false);

            auto page = addPage(select);
            auto tab = page->getTab();
            tab->load(v.second);
            page->load(v.second);
        }
    } catch (boost::property_tree::ptree_error &) {
        // can't read tabs
    }

    if (_pages.size() == 0) {
        // No pages saved, show default stuff
        loadDefaults();
    }
}

void Notebook::save(boost::property_tree::ptree &tree)
{
    boost::property_tree::ptree tabs;

    // Iterate through all tabs and add them to our tabs property thing
    for (const auto &page : _pages) {
        boost::property_tree::ptree pTab = page->getTab()->save();

        boost::property_tree::ptree pChats = page->save();

        if (pChats.size() > 0) {
            pTab.add_child("columns", pChats);
        }

        tabs.push_back(std::make_pair("", pTab));
    }

    tree.add_child("tabs", tabs);
}

void Notebook::loadDefaults()
{
    addPage();
}

}  // namespace widgets
}  // namespace chatterino
