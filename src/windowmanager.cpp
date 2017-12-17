#include "windowmanager.hpp"
#include "appdatapath.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"

#include <QDebug>
#include <QStandardPaths>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace chatterino {
WindowManager *WindowManager::instance = nullptr;

WindowManager::WindowManager(ChannelManager &_channelManager, ColorScheme &_colorScheme)
    : channelManager(_channelManager)
    , colorScheme(_colorScheme)
{
    WindowManager::instance = this;
}

void WindowManager::initMainWindow()
{
    this->selectedWindow = this->mainWindow =
        new widgets::Window(this->channelManager, this->colorScheme, true);
}

static const std::string &getSettingsPath()
{
    static std::string path = (Path::getAppdataPath() + "uilayout.json").toStdString();

    return path;
}

void WindowManager::layoutVisibleChatWidgets(Channel *channel)
{
    this->layout();
}

void WindowManager::repaintVisibleChatWidgets(Channel *channel)
{
    if (this->mainWindow != nullptr) {
        this->mainWindow->repaintVisibleChatWidgets(channel);
    }
}

void WindowManager::repaintGifEmotes()
{
    this->repaintGifs();
}

// void WindowManager::updateAll()
//{
//    if (this->mainWindow != nullptr) {
//        this->mainWindow->update();
//    }
//}

widgets::Window &WindowManager::getMainWindow()
{
    return *this->mainWindow;
}

widgets::Window &WindowManager::getSelectedWindow()
{
    return *this->selectedWindow;
}

widgets::Window &WindowManager::createWindow()
{
    auto *window = new widgets::Window(this->channelManager, this->colorScheme, false);

    window->loadDefaults();

    this->windows.push_back(window);

    return *window;
}

int WindowManager::windowCount()
{
    return this->windows.size();
}

widgets::Window *WindowManager::windowAt(int index)
{
    if (index < 0 || index >= this->windows.size()) {
        return nullptr;
    }
    qDebug() << "getting window at bad index" << index;

    return this->windows.at(index);
}

void WindowManager::load()
{
    const auto &settingsPath = getSettingsPath();
    boost::property_tree::ptree tree;

    try {
        boost::property_tree::read_json(settingsPath, tree);
    } catch (const boost::property_tree::json_parser_error &ex) {
        qDebug() << "Error using property_tree::readJson: " << QString::fromStdString(ex.message());

        getMainWindow().loadDefaults();

        return;
    }

    // Read a list of windows
    try {
        BOOST_FOREACH (const boost::property_tree::ptree::value_type &v,
                       tree.get_child("windows.")) {
            qDebug() << QString::fromStdString(v.first.data());
            const auto &type = v.second.get<std::string>("type", "unknown");

            if (type == "main") {
                getMainWindow().load(v.second);
            } else {
                qDebug() << "Unhandled window type: " << type.c_str();
            }
        }
    } catch (boost::property_tree::ptree_error &) {
        // can't read windows
    }

    // if the main window was not loaded properly, load defaults
    if (!getMainWindow().isLoaded()) {
        getMainWindow().loadDefaults();
    }

    // If there are no windows, create a default main window
}

void WindowManager::save()
{
    auto &settingsPath = getSettingsPath();
    boost::property_tree::ptree tree;

    // Create windows array
    boost::property_tree::ptree windows;

    {
        // save main window
        auto child = getMainWindow().save();
        windows.push_back(std::make_pair("", child));
    }

    // TODO: iterate through rest of windows and add them to the "windows" ptree

    tree.add_child("windows", windows);

    boost::property_tree::write_json(settingsPath, tree);
}

}  // namespace chatterino
