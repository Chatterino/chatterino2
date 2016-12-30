#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chatwidget.h"
#include "notebook.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    notebook(this)
{
    //ui->setupUi(this);

    //ChatWidget widget = new ChatWidget(this);

    //this->notebook = new Notebook(this);

    setCentralWidget(&this->notebook);

    this->notebook.addPage();
    this->notebook.addPage();
    this->notebook.addPage();
}

MainWindow::~MainWindow()
{
    delete ui;
}
