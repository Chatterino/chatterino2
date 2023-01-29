#include "PluginController.hpp"

#include "common/QLogging.hpp"

#include <QApplication>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <qfileinfo.h>
}

namespace chatterino {

void PluginController::initialize(Settings &settings, Paths &paths)
{
    (void)(settings);

    auto dir = QDir(paths.pluginsDirectory);
    qCDebug(chatterinoLua) << "loading plugins from " << dir;
    for (const auto &info : dir.entryInfoList())
    {
        if (info.isDir())
        {
            // look for index.lua
            auto pluginDir = QDir(info.absoluteFilePath());
            auto index = QFileInfo(pluginDir.filePath("index.lua"));
            qCDebug(chatterinoLua)
                << "trying" << info << "," << index << "at" << pluginDir;
            if (!index.exists())
            {
                qCDebug(chatterinoLua)
                    << "Missing index.lua in plugin directory" << pluginDir;
                continue;
            }
            qCDebug(chatterinoLua) << "found index.lua, running it!";

            this->load(index, pluginDir);
        }
    }
    //    QApplication::exit();
}

void PluginController::load(QFileInfo index, QDir pluginDir)
{
    qCDebug(chatterinoLua) << "Running lua file" << index;
    lua_State *l = luaL_newstate();
    luaL_openlibs(l);

    luaL_dofile(l, index.absoluteFilePath().toStdString().c_str());
    lua_close(l);
}

};  // namespace chatterino
