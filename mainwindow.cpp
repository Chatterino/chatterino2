#include "mainwindow.h"
#include <QPalette>
#include "chatwidget.h"
#include "colorscheme.h"
#include "notebook.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , notebook(this)
{
    setCentralWidget(&this->notebook);

    this->notebook.addPage();
    this->notebook.addPage();
    this->notebook.addPage();

    QPalette palette;
    palette.setColor(QPalette::Background,
                     ColorScheme::instance().TabPanelBackground);
    setPalette(palette);

    resize(1280, 800);
}

MainWindow::~MainWindow()
{
}

void
MainWindow::layoutVisibleChatWidgets()
{
    auto *page = notebook.selected();

    if (page == NULL) {
        return;
    }

    const std::vector<ChatWidget *> &widgets = page->chatWidgets();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        ChatWidget *widget = *it;

        if (widget->view().layoutMessages()) {
            widget->repaint();
        }
    }
}

void
MainWindow::repaintVisibleChatWidgets()
{
    auto *page = notebook.selected();

    if (page == NULL) {
        return;
    }

    const std::vector<ChatWidget *> &widgets = page->chatWidgets();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        ChatWidget *widget = *it;

        widget->view().layoutMessages();
        widget->repaint();
    }
}
