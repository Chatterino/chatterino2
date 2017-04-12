#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "widgets/notebook.h"
#include "widgets/titlebar.h"

#include <platform/borderless/qwinwidget.h>
#include <QMainWindow>
#include <boost/property_tree/ptree.hpp>

namespace chatterino {
namespace widgets {

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void layoutVisibleChatWidgets(Channel *channel = nullptr);
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();

    void load(const boost::property_tree::ptree &tree);
    boost::property_tree::ptree save();
    void loadDefaults();

    bool isLoaded() const;

    Notebook &getNotebook();

private:
    Notebook _notebook;
    bool _loaded;
    TitleBar _titleBar;
};

}  // namespace widgets
}  // namespace chatterino

#endif  // MAINWINDOW_H
