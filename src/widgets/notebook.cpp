#include "widgets/notebook.hpp"
#include "colorscheme.hpp"
#include "widgets/mainwindow.hpp"
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

Notebook::Notebook(ChannelManager &_channelManager, MainWindow *parent)
    : BaseWidget(parent)
    , channelManager(_channelManager)
    , completionManager(parent->completionManager)
    , addButton(this)
    , settingsButton(this)
    , userButton(this)
{
    this->connect(&this->settingsButton, SIGNAL(clicked()), this, SLOT(settingsButtonClicked()));
    this->connect(&this->userButton, SIGNAL(clicked()), this, SLOT(usersButtonClicked()));
    this->connect(&this->addButton, SIGNAL(clicked()), this, SLOT(addPageButtonClicked()));

    this->settingsButton.icon = NotebookButton::IconSettings;

    this->userButton.move(24, 0);
    this->userButton.icon = NotebookButton::IconUser;

    SettingsManager::getInstance().hidePreferencesButton.valueChanged.connect(
        [this](const bool &) { this->performLayout(); });
    SettingsManager::getInstance().hideUserButton.valueChanged.connect(
        [this](const bool &) { this->performLayout(); });
}

NotebookPage *Notebook::addPage(bool select)
{
    auto tab = new NotebookTab(this);
    auto page = new NotebookPage(this->channelManager, this, tab);

    tab->show();

    if (select || this->pages.count() == 0) {
        this->select(page);
    }

    this->pages.append(page);

    this->performLayout();

    return page;
}

void Notebook::removePage(NotebookPage *page)
{
    int index = this->pages.indexOf(page);

    if (this->pages.size() == 1) {
        select(nullptr);
    } else if (index == this->pages.count() - 1) {
        select(this->pages[index - 1]);
    } else {
        select(this->pages[index + 1]);
    }

    page->getTab()->deleteLater();
    page->deleteLater();

    this->pages.removeOne(page);

    if (this->pages.size() == 0) {
        addPage();
    }

    this->performLayout();
}

void Notebook::select(NotebookPage *page)
{
    if (page == this->selectedPage) {
        return;
    }

    if (page != nullptr) {
        page->setHidden(false);
        page->getTab()->setSelected(true);
        page->getTab()->raise();
    }

    if (this->selectedPage != nullptr) {
        this->selectedPage->setHidden(true);
        this->selectedPage->getTab()->setSelected(false);
    }

    this->selectedPage = page;

    this->performLayout();
}

NotebookPage *Notebook::tabAt(QPoint point, int &index)
{
    int i = 0;

    for (auto *page : this->pages) {
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
    this->pages.move(this->pages.indexOf(page), index);

    this->performLayout();
}

void Notebook::nextTab()
{
    if (this->pages.size() <= 1) {
        return;
    }

    int index = (this->pages.indexOf(this->selectedPage) + 1) % this->pages.size();

    this->select(this->pages[index]);
}

void Notebook::previousTab()
{
    if (this->pages.size() <= 1) {
        return;
    }

    int index = (this->pages.indexOf(this->selectedPage) - 1);

    if (index < 0) {
        index += this->pages.size();
    }

    this->select(this->pages[index]);
}

void Notebook::performLayout(bool animated)
{
    int x = 0, y = 0;
    float scale = this->getDpiMultiplier();

    if (SettingsManager::getInstance().hidePreferencesButton.get()) {
        this->settingsButton.hide();
    } else {
        this->settingsButton.show();
        x += settingsButton.width();
    }
    if (SettingsManager::getInstance().hideUserButton.get()) {
        this->userButton.hide();
    } else {
        this->userButton.move(x, 0);
        this->userButton.show();
        x += userButton.width();
    }

    int tabHeight = static_cast<int>(24 * scale);
    bool first = true;

    for (auto &i : this->pages) {
        if (!first &&
            (i == this->pages.last() ? tabHeight : 0) + x + i->getTab()->width() > width()) {
            y += i->getTab()->height();
            i->getTab()->moveAnimated(QPoint(0, y), animated);
            x = i->getTab()->width();
        } else {
            i->getTab()->moveAnimated(QPoint(x, y), animated);
            x += i->getTab()->width();
        }

        first = false;
    }

    this->addButton.move(x, y);

    if (this->selectedPage != nullptr) {
        this->selectedPage->move(0, y + tabHeight);
        this->selectedPage->resize(width(), height() - y - tabHeight);
    }
}

void Notebook::resizeEvent(QResizeEvent *)
{
    float scale = this->getDpiMultiplier();

    this->settingsButton.resize(static_cast<int>(24 * scale), static_cast<int>(24 * scale));
    this->userButton.resize(static_cast<int>(24 * scale), static_cast<int>(24 * scale));
    this->addButton.resize(static_cast<int>(24 * scale), static_cast<int>(24 * scale));

    for (auto &i : this->pages) {
        i->getTab()->calcSize();
    }

    this->performLayout(false);
}

void Notebook::settingsButtonClicked()
{
    QTimer::singleShot(80, [this] { SettingsDialog::showDialog(); });
}

void Notebook::usersButtonClicked()
{
}

void Notebook::addPageButtonClicked()
{
    QTimer::singleShot(80, [this] { this->addPage(true); });
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

    if (this->pages.size() == 0) {
        // No pages saved, show default stuff
        loadDefaults();
    }
}

void Notebook::save(boost::property_tree::ptree &tree)
{
    boost::property_tree::ptree tabs;

    // Iterate through all tabs and add them to our tabs property thing
    for (const auto &page : this->pages) {
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
