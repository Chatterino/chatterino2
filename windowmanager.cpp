#include "windowmanager.h"
#include "appdatapath.h"

#include <QDebug>
#include <QStandardPaths>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace chatterino {
WindowManager WindowManager::instance;

WindowManager::WindowManager()
    : _mainWindow(nullptr)
{
}

static const std::string &getSettingsPath()
{
    static std::string path = (Path::getAppdataPath() + "uilayout.json").toStdString();

    return path;
}

void WindowManager::layoutVisibleChatWidgets(Channel *channel)
{
    if (_mainWindow != nullptr) {
        _mainWindow->layoutVisibleChatWidgets(channel);
    }
}

void WindowManager::repaintVisibleChatWidgets(Channel *channel)
{
    if (_mainWindow != nullptr) {
        _mainWindow->repaintVisibleChatWidgets(channel);
    }
}

void WindowManager::repaintGifEmotes()
{
    if (_mainWindow != nullptr) {
        _mainWindow->repaintGifEmotes();
    }
}

void WindowManager::updateAll()
{
    if (_mainWindow != nullptr) {
        _mainWindow->update();
    }
}

widgets::MainWindow &WindowManager::getMainWindow()
{
    std::lock_guard<std::mutex> lock(_windowMutex);

    if (_mainWindow == nullptr) {
        _mainWindow = new widgets::MainWindow();
    }

    return *_mainWindow;
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
