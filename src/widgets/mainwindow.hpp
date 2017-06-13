#pragma once

#include "widgets/notebook.hpp"
#include "widgets/titlebar.hpp"

#ifdef USEWINSDK
#include <platform/borderless/qwinwidget.h>
#endif

#include <QMainWindow>
#include <boost/property_tree/ptree.hpp>

namespace chatterino {

class ChannelManager;

namespace widgets {

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(ChannelManager &_channelManager, QWidget *parent = nullptr);
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
    ChannelManager &channelManager;

    Notebook notebook;
    bool _loaded;
    TitleBar _titleBar;
};

}  // namespace widgets
}  // namespace chatterino
