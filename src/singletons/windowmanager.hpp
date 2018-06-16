#pragma once

#include "widgets/splitcontainer.hpp"
#include "widgets/window.hpp"

namespace chatterino {
// namespace widgets {
// struct SplitContainer::Node;
//}
namespace singletons {

class WindowManager
{
public:
    WindowManager();

    ~WindowManager() = delete;

    void showSettingsDialog();
    void showAccountSelectPopup(QPoint point);

    void layoutChannelViews(Channel *channel = nullptr);
    void forceLayoutChannelViews();
    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();
    // void updateAll();

    widgets::Window &getMainWindow();
    widgets::Window &getSelectedWindow();
    widgets::Window &createWindow(widgets::Window::WindowType type);

    int windowCount();
    widgets::Window *windowAt(int index);

    void save();
    void initialize();
    void closeAll();

    int getGeneration() const;
    void incGeneration();

    pajlada::Signals::NoArgSignal repaintGifs;
    pajlada::Signals::Signal<Channel *> layout;

    static const int uiScaleMin;
    static const int uiScaleMax;
    static int clampUiScale(int scale);
    static float getUiScaleValue();
    static float getUiScaleValue(int scale);

private:
    bool initialized = false;

    std::atomic<int> generation{0};

    std::vector<widgets::Window *> windows;

    widgets::Window *mainWindow = nullptr;
    widgets::Window *selectedWindow = nullptr;

    void encodeNodeRecusively(widgets::SplitContainer::Node *node, QJsonObject &obj);

public:
    static void encodeChannel(IndirectChannel channel, QJsonObject &obj);
    static IndirectChannel decodeChannel(const QJsonObject &obj);
};

}  // namespace singletons
}  // namespace chatterino
