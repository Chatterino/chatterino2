#include "widgets/mainwindow.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"
#include "widgets/chatwidget.hpp"
#include "widgets/notebook.hpp"
#include "widgets/settingsdialog.hpp"

#include <QDebug>
#include <QPalette>
#include <QShortcut>
#include <QVBoxLayout>
#include <boost/foreach.hpp>

#ifdef USEWINSDK
#include "Windows.hpp"
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

    // Initialize program-wide hotkeys
    {
        // CTRL+P: Open Settings Dialog
        auto shortcut = new QShortcut(QKeySequence("CTRL+P"), this);
        connect(shortcut, &QShortcut::activated, []() {
            SettingsDialog::showDialog();  //
        });
    }
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
