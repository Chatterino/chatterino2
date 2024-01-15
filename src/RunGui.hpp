#pragma once

class QApplication;

namespace chatterino {

class Args;
class Paths;
class Settings;
class Updates;

void runGui(QApplication &a, const Paths &paths, Settings &settings,
            const Args &args, Updates &updates);

}  // namespace chatterino
