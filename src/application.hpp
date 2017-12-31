#pragma once

#include "singletons/ircmanager.hpp"
#include "singletons/resourcemanager.hpp"

#include <QApplication>

namespace chatterino {

class Application
{
public:
    Application();
    ~Application();

    int run(QApplication &qtApp);

private:
    void save();
};

}  // namespace chatterino
