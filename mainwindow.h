#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "notebook.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    Notebook notebook;

    void layoutVisibleChatWidgets();
    void repaintVisibleChatWidgets();
};

#endif  // MAINWINDOW_H
