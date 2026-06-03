#pragma once

#include <QString>

namespace chatterino::embed {

class App;
struct CreateAppArgs {
    QString rootDirectory;
    bool saveSettingsOnExit = false;
};

App *createAppPrivate(const CreateAppArgs &args);

}  // namespace chatterino::embed
