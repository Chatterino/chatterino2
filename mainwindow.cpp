#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chatwidget.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //ui->setupUi(this);

    //ChatWidget widget = new ChatWidget(this);

    setCentralWidget(new ChatWidget(this));
}

MainWindow::~MainWindow()
{
    delete ui;
}
