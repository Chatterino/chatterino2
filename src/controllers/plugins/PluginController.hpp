#pragma once

#include "common/Singleton.hpp"
#include "singletons/Paths.hpp"

#include <QDebug>
#include <QDir>

namespace chatterino {

class PluginController : public Singleton
{
public:
    void initialize(Settings &settings, Paths &paths) override;
    void save() override{};

private:
    void load(QFileInfo index, QDir pluginDir);
};

};  // namespace chatterino
