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
