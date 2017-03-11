#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "widgets/notebook.h"

#include <QMainWindow>
#include <boost/property_tree/ptree.hpp>

namespace chatterino {
namespace widgets {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    Notebook notebook;

    void layoutVisibleChatWidgets(Channel *channel = NULL);
    void repaintVisibleChatWidgets(Channel *channel = NULL);
    void repaintGifEmotes();

    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
    void loadDefaults();

    bool
    isLoaded() const
    {
        return this->loaded;
    }

private:
    bool loaded = false;
};

}  // namespace widgets
}  // namespace chatterino

#endif  // MAINWINDOW_H
