#pragma once

class QApplication;

namespace chatterino {

class Args;
class Paths;
class Settings;

void runGui(QApplication &a, const Paths &paths, Settings &settings,
            const Args &args);

}  // namespace chatterino
