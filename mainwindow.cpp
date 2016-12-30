#include "mainwindow.h"
#include "chatwidget.h"
#include "notebook.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    notebook(this)
{
    setCentralWidget(&this->notebook);

    this->notebook.addPage();
    this->notebook.addPage();
    this->notebook.addPage();
}

MainWindow::~MainWindow()
{

}
