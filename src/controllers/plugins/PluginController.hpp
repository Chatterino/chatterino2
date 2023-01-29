#pragma once

#include "common/Singleton.hpp"
#include "singletons/Paths.hpp"

#include <QDir>
#include <QFileInfo>
#include <QString>

#include <map>
#include <memory>
#include <utility>
#include <vector>

class lua_State;

namespace chatterino {

//class Registration
//{
//public:
//    enum Type {
//        COMMAND,
//    };
//
//    Type type;
//    QString name;
//    const char *receiverFunctionName;
//};

class Plugin
{
public:
    QString name;
    Plugin(QString name, lua_State *state)
        : name(std::move(name))
        , state_(state)
    {
    }

private:
    lua_State *state_;

    friend class PluginController;
};

class PluginController : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;
    void save() override{};
    void callEvery(const QString &functionName);

private:
    void load(QFileInfo index, QDir pluginDir);
    void loadChatterinoLib(lua_State *l);

    std::map<QString, std::unique_ptr<Plugin>> plugins;
};

};  // namespace chatterino
