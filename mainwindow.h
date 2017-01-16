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

    void layoutVisibleChatWidgets(Channel *channel = NULL);
    void repaintVisibleChatWidgets(Channel *channel = NULL);
};

#endif  // MAINWINDOW_H
