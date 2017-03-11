#include "widgets/mainwindow.h"
#include "colorscheme.h"
#include "widgets/chatwidget.h"
#include "widgets/notebook.h"

#include <QDebug>
#include <QPalette>
#include <boost/foreach.hpp>

namespace chatterino {
namespace widgets {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , notebook(this)
{
    setCentralWidget(&this->notebook);

    QPalette palette;
    palette.setColor(QPalette::Background,
                     ColorScheme::getInstance().TabPanelBackground);
    setPalette(palette);

    resize(1280, 800);
}

MainWindow::~MainWindow()
{
}

void
MainWindow::layoutVisibleChatWidgets(Channel *channel)
{
    auto *page = notebook.getSelectedPage();

    if (page == NULL) {
        return;
    }

    const std::vector<ChatWidget *> &widgets = page->getChatWidgets();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        ChatWidget *widget = *it;

        if (channel == NULL || channel == widget->getChannel().get()) {
            if (widget->getView().layoutMessages()) {
                widget->getView().update();
            }
        }
    }
}

void
MainWindow::repaintVisibleChatWidgets(Channel *channel)
{
    auto *page = notebook.getSelectedPage();

    if (page == NULL) {
        return;
    }

    const std::vector<ChatWidget *> &widgets = page->getChatWidgets();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        ChatWidget *widget = *it;

        if (channel == NULL || channel == widget->getChannel().get()) {
            widget->getView().layoutMessages();
            widget->getView().update();
        }
    }
}

void
MainWindow::repaintGifEmotes()
{
    auto *page = notebook.getSelectedPage();

    if (page == NULL) {
        return;
    }

    const std::vector<ChatWidget *> &widgets = page->getChatWidgets();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        ChatWidget *widget = *it;

        widget->getView().updateGifEmotes();
    }
}

void
MainWindow::load(const boost::property_tree::ptree &tree)
{
    this->notebook.load(tree);

    this->loaded = true;
}

boost::property_tree::ptree
MainWindow::save()
{
    boost::property_tree::ptree child;

    child.put("type", "main");

    this->notebook.save(child);

    return child;
}

void
MainWindow::loadDefaults()
{
    this->notebook.loadDefaults();

    this->loaded = true;
}

}  // namespace widgets
}  // namespace chatterino
