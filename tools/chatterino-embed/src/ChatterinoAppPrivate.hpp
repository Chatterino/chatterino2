#pragma once

#include <QString>

namespace chatterino::embed {

class ChatterinoApp;
struct CreateAppArgs {
    QString rootDirectory;
    bool saveSettingsOnExit = false;
};

ChatterinoApp *createAppPrivate(const CreateAppArgs &args);

}  // namespace chatterino::embed
