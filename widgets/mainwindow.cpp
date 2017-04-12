#include "widgets/mainwindow.h"
#include "colorscheme.h"
#include "settingsmanager.h"
#include "widgets/chatwidget.h"
#include "widgets/notebook.h"

#include <QDebug>
#include <QPalette>
#include <QVBoxLayout>
#include <boost/foreach.hpp>

#ifdef USEWINSDK
#include "Windows.h"
#endif

namespace chatterino {
namespace widgets {

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , _notebook(this)
    , _loaded(false)
    , _titleBar()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // add titlebar
    //    if (SettingsManager::getInstance().useCustomWindowFrame.get()) {
    //        layout->addWidget(&_titleBar);
    //    }

    layout->addWidget(&_notebook);
    setLayout(layout);

    // set margin
    //    if (SettingsManager::getInstance().useCustomWindowFrame.get()) {
    //        layout->setMargin(1);
    //    } else {
    layout->setMargin(0);
    //    }

    QPalette palette;
    palette.setColor(QPalette::Background, ColorScheme::getInstance().TabPanelBackground);
    setPalette(palette);

    resize(1280, 800);
}

MainWindow::~MainWindow()
{
}

void MainWindow::layoutVisibleChatWidgets(Channel *channel)
{
    auto *page = _notebook.getSelectedPage();

    if (page == NULL) {
        return;
    }

    const std::vector<ChatWidget *> &widgets = page->getChatWidgets();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        ChatWidget *widget = *it;

        if (channel == NULL || channel == widget->getChannel().get()) {
            widget->layoutMessages();
        }
    }
}

void MainWindow::repaintVisibleChatWidgets(Channel *channel)
{
    auto *page = _notebook.getSelectedPage();

    if (page == NULL) {
        return;
    }

    const std::vector<ChatWidget *> &widgets = page->getChatWidgets();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        ChatWidget *widget = *it;

        if (channel == NULL || channel == widget->getChannel().get()) {
            widget->layoutMessages();
        }
    }
}

void MainWindow::repaintGifEmotes()
{
    auto *page = _notebook.getSelectedPage();

    if (page == NULL) {
        return;
    }

    const std::vector<ChatWidget *> &widgets = page->getChatWidgets();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        ChatWidget *widget = *it;

        widget->updateGifEmotes();
    }
}

void MainWindow::load(const boost::property_tree::ptree &tree)
{
    this->_notebook.load(tree);

    _loaded = true;
}

boost::property_tree::ptree MainWindow::save()
{
    boost::property_tree::ptree child;

    child.put("type", "main");

    _notebook.save(child);

    return child;
}

void MainWindow::loadDefaults()
{
    _notebook.loadDefaults();

    _loaded = true;
}

bool MainWindow::isLoaded() const
{
    return _loaded;
}

Notebook &MainWindow::getNotebook()
{
    return _notebook;
}
}  // namespace widgets
}  // namespace chatterino
