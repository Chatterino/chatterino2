#include "windows.h"

#include <QDebug>
#include <QStandardPaths>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace chatterino {

static const std::string &
getSettingsPath()
{
    static std::string path =
        (QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
         "/windows.json")
            .toStdString();

    return path;
}

QMutex Windows::windowMutex;

widgets::MainWindow *Windows::mainWindow(nullptr);

void
Windows::layoutVisibleChatWidgets(Channel *channel)
{
    if (Windows::mainWindow != nullptr) {
        Windows::mainWindow->layoutVisibleChatWidgets(channel);
    }
}

void
Windows::repaintVisibleChatWidgets(Channel *channel)
{
    if (Windows::mainWindow != nullptr) {
        Windows::mainWindow->repaintVisibleChatWidgets(channel);
    }
}

void
Windows::load()
{
    const auto &settingsPath = getSettingsPath();
    boost::property_tree::ptree tree;

    try {
        boost::property_tree::read_json(settingsPath, tree);
    } catch (const boost::property_tree::json_parser_error &ex) {
        qDebug() << "Error using property_tree::readJson: "
                 << QString::fromStdString(ex.message());

        Windows::getMainWindow().loadDefaults();

        return;
    }

    // Read a list of windows
    try {
        BOOST_FOREACH (const boost::property_tree::ptree::value_type &v,
                       tree.get_child("windows.")) {
            qDebug() << QString::fromStdString(v.first.data());
            const auto &type = v.second.get<std::string>("type", "unknown");

            if (type == "main") {
                Windows::getMainWindow().load(v.second);
            } else {
                qDebug() << "Unhandled window type: " << type.c_str();
            }
        }
    } catch (boost::property_tree::ptree_error &) {
        // can't read windows
    }

    // if the main window was not loaded properly, load defaults
    if (!Windows::getMainWindow().isLoaded()) {
        Windows::getMainWindow().loadDefaults();
    }

    // If there are no windows, create a default main window
}

void
Windows::save()
{
    const auto &settingsPath = getSettingsPath();
    boost::property_tree::ptree tree;

    // Create windows array
    boost::property_tree::ptree windows;

    {
        // save main window
        auto child = Windows::getMainWindow().save();
        windows.push_back(std::make_pair("", child));
    }

    // TODO: iterate through rest of windows and add them to the "windows" ptree

    tree.add_child("windows", windows);

    boost::property_tree::write_json(settingsPath, tree);
}

}  // namespace chatterino
