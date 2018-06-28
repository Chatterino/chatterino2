#pragma once

#include "widgets/Window.hpp"
#include "widgets/splits/SplitContainer.hpp"

namespace chatterino {

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

    Window &getMainWindow();
    Window &getSelectedWindow();
    Window &createWindow(Window::WindowType type);

    int windowCount();
    Window *windowAt(int index);

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

    MessageElement::Flags getWordFlags();
    void updateWordTypeMask();

    pajlada::Signals::NoArgSignal wordFlagsChanged;

private:
    bool initialized = false;

    std::atomic<int> generation{0};

    std::vector<Window *> windows;

    Window *mainWindow = nullptr;
    Window *selectedWindow = nullptr;

    void encodeNodeRecusively(SplitContainer::Node *node, QJsonObject &obj);

    MessageElement::Flags wordFlags = MessageElement::Default;
    pajlada::Settings::SettingListener wordFlagsListener;

public:
    static void encodeChannel(IndirectChannel channel, QJsonObject &obj);
    static IndirectChannel decodeChannel(const QJsonObject &obj);
};

}  // namespace chatterino
