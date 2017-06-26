#include "widgets/notebook.hpp"
#include "colorscheme.hpp"
#include "widgets/notebookbutton.hpp"
#include "widgets/notebookpage.hpp"
#include "widgets/notebooktab.hpp"
#include "widgets/settingsdialog.hpp"

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

Notebook::Notebook(ChannelManager &_channelManager, BaseWidget *parent)
    : BaseWidget(parent)
    , channelManager(_channelManager)
    , addButton(this)
    , settingsButton(this)
    , userButton(this)
    , selectedPage(nullptr)
{
    connect(&settingsButton, SIGNAL(clicked()), this, SLOT(settingsButtonClicked()));
    connect(&userButton, SIGNAL(clicked()), this, SLOT(usersButtonClicked()));
    connect(&addButton, SIGNAL(clicked()), this, SLOT(addPageButtonClicked()));

    settingsButton.resize(24, 24);
    settingsButton.icon = NotebookButton::IconSettings;

    userButton.resize(24, 24);
    userButton.move(24, 0);
    userButton.icon = NotebookButton::IconUser;

    addButton.resize(24, 24);

    SettingsManager::getInstance().hidePreferencesButton.valueChanged.connect(
        [this](const bool &) { performLayout(); });
    SettingsManager::getInstance().hideUserButton.valueChanged.connect(
        [this](const bool &) { performLayout(); });
}

NotebookPage *Notebook::addPage(bool select)
{
    auto tab = new NotebookTab(this);
    auto page = new NotebookPage(this->channelManager, this, tab);

    tab->show();

    if (select || pages.count() == 0) {
        this->select(page);
    }

    pages.append(page);

    performLayout();

    return page;
}

void Notebook::removePage(NotebookPage *page)
{
    int index = pages.indexOf(page);

    if (pages.size() == 1) {
        select(nullptr);
    } else if (index == pages.count() - 1) {
        select(pages[index - 1]);
    } else {
        select(pages[index + 1]);
    }

    delete page->getTab();
    delete page;

    pages.removeOne(page);

    if (pages.size() == 0) {
        addPage();
    }

    performLayout();
}

void Notebook::select(NotebookPage *page)
{
    if (page == selectedPage)
        return;

    if (page != nullptr) {
        page->setHidden(false);
        page->getTab()->setSelected(true);
        page->getTab()->raise();
    }

    if (selectedPage != nullptr) {
        selectedPage->setHidden(true);
        selectedPage->getTab()->setSelected(false);
    }

    selectedPage = page;

    performLayout();
}

NotebookPage *Notebook::tabAt(QPoint point, int &index)
{
    int i = 0;

    for (auto *page : pages) {
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
    pages.move(pages.indexOf(page), index);

    performLayout();
}

void Notebook::performLayout(bool animated)
{
    int x = 0, y = 0;

    if (SettingsManager::getInstance().hidePreferencesButton.get()) {
        settingsButton.hide();
    } else {
        settingsButton.show();
        x += 24;
    }
    if (SettingsManager::getInstance().hideUserButton.get()) {
        userButton.hide();
    } else {
        userButton.move(x, 0);
        userButton.show();
        x += 24;
    }

    int tabHeight = 16;
    bool first = true;

    for (auto &i : pages) {
        tabHeight = i->getTab()->height();

        if (!first && (i == pages.last() ? tabHeight : 0) + x + i->getTab()->width() > width()) {
            y += i->getTab()->height();
            i->getTab()->moveAnimated(QPoint(0, y), animated);
            x = i->getTab()->width();
        } else {
            i->getTab()->moveAnimated(QPoint(x, y), animated);
            x += i->getTab()->width();
        }

        first = false;
    }

    addButton.move(x, y);

    if (selectedPage != nullptr) {
        selectedPage->move(0, y + tabHeight);
        selectedPage->resize(width(), height() - y - tabHeight);
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

    if (pages.size() == 0) {
        // No pages saved, show default stuff
        loadDefaults();
    }
}

void Notebook::save(boost::property_tree::ptree &tree)
{
    boost::property_tree::ptree tabs;

    // Iterate through all tabs and add them to our tabs property thing
    for (const auto &page : pages) {
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
