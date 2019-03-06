#pragma once

class QApplication;

namespace chatterino
{
    class Paths;
    class Settings;

    void runGui(QApplication& a, Paths& paths, Settings& settings);
}  // namespace chatterino
