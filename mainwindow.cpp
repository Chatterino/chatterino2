#include <QPalette>
#include "mainwindow.h"
#include "chatwidget.h"
#include "notebook.h"
#include "colorscheme.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    notebook(this)
{
    setCentralWidget(&this->notebook);

    this->notebook.addPage();
    this->notebook.addPage();
    this->notebook.addPage();

    QPalette palette;
    palette.setColor(QPalette::Background, ColorScheme::getInstance().TabPanelBackground);
    setPalette(palette);

    resize(1280, 800);
}

MainWindow::~MainWindow()
{

}
