#pragma once

#include "common/FlagsEnum.hpp"
#include "util/SignalListener.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <pajlada/settings/settinglistener.hpp>
#include <QPoint>
#include <QTimer>

#include <memory>
#include <set>

namespace chatterino {

class Settings;
class Paths;
class Window;
class ChannelView;
class IndirectChannel;
class Split;
struct SplitDescriptor;
class Channel;
using ChannelPtr = std::shared_ptr<Channel>;
struct Message;
using MessagePtr = std::shared_ptr<const Message>;
class WindowLayout;
class Theme;
class Fonts;

enum class MessageElementFlag : int64_t;
using MessageElementFlags = FlagsEnum<MessageElementFlag>;
enum class WindowType;

enum class SettingsDialogPreference;
class FramelessEmbedWindow;

class WindowManager final
{
    Theme &themes;

public:
    static const QString WINDOW_LAYOUT_FILENAME;

    explicit WindowManager(const Paths &paths, Settings &settings,
                           Theme &themes_, Fonts &fonts);
    ~WindowManager();

    WindowManager(const WindowManager &) = delete;
    WindowManager(WindowManager &&) = delete;
    WindowManager &operator=(const WindowManager &) = delete;
    WindowManager &operator=(WindowManager &&) = delete;

    static void encodeTab(SplitContainer *tab, bool isSelected,
                          QJsonObject &obj);
    static void encodeChannel(IndirectChannel channel, QJsonObject &obj);
    static void encodeFilters(Split *split, QJsonArray &arr);
    static IndirectChannel decodeChannel(const SplitDescriptor &descriptor);

    void showSettingsDialog(
        QWidget *parent,
        SettingsDialogPreference preference = SettingsDialogPreference());

    // Show the account selector widget at point
    void showAccountSelectPopup(QPoint point);

    // Tell a channel (or all channels if channel is nullptr) to redo their
    // layout
    void layoutChannelViews(Channel *channel = nullptr);

    // Force all channel views to redo their layout
    // This is called, for example, when the emote scale or timestamp format has
    // changed
    void forceLayoutChannelViews();

    // Tell a channel (or all channels if channel is nullptr) to invalidate all paint buffers
    void invalidateChannelViewBuffers(Channel *channel = nullptr);

    void repaintVisibleChatWidgets(Channel *channel = nullptr);
    void repaintGifEmotes();

    Window &getMainWindow();

    // Returns a pointer to the last selected window.
    // Edge cases:
    //  - If the application was not focused since the start, this will return a pointer to the main window.
    //  - If the window was closed this points to the main window.
    //  - If the window was unfocused since being selected, this function will still return it.
    Window *getLastSelectedWindow() const;

    Window &createWindow(WindowType type, bool show = true,
                         QWidget *parent = nullptr);

    // Use this method if you want to open a "new" channel in a popup. If you want to popup an
    // existing Split or SplitContainer, consider using Split::popup() or SplitContainer::popup().
    Window &openInPopup(ChannelPtr channel);

    void select(Split *split);
    void select(SplitContainer *container);
    /**
     * Scrolls to the message in a split that's not
     * a mentions view and focuses the split.
     *
     * @param message Message to scroll to.
     */
    void scrollToMessage(const MessagePtr &message);

    QRect emotePopupBounds() const;
    void setEmotePopupBounds(QRect bounds);

    // Set up some final signals & actually show the windows
    void initialize();
    void save();
    void closeAll();

    int getGeneration() const;
    void incGeneration();

    MessageElementFlags getWordFlags();
    void updateWordTypeMask();

    // Sends an alert to the main window
    // It reads the `longAlert` setting to decide whether the alert will expire
    // or not
    void sendAlert();

    // Queue up a save in the next 10 seconds
    // If a save was already queued up, we reset the to happen in 10 seconds
    // again
    void queueSave();

    /// Toggles the inertia in all open overlay windows
    void toggleAllOverlayInertia();

    std::set<QString> getVisibleChannelNames() const;

    /// Signals
    pajlada::Signals::NoArgSignal gifRepaintRequested;

    // This signal fires whenever views rendering a channel, or all views if the
    // channel is a nullptr, need to redo their layout
    pajlada::Signals::Signal<Channel *> layoutRequested;
    // This signal fires whenever views rendering a channel, or all views if the
    // channel is a nullptr, need to invalidate their paint buffers
    pajlada::Signals::Signal<Channel *> invalidateBuffersRequested;

    pajlada::Signals::NoArgSignal wordFlagsChanged;

    pajlada::Signals::Signal<Split *> selectSplit;
    pajlada::Signals::Signal<SplitContainer *> selectSplitContainer;
    pajlada::Signals::Signal<const MessagePtr &> scrollToMessageSignal;

private:
    static void encodeNodeRecursively(SplitContainer::Node *node,
                                      QJsonObject &obj);

    // Load window layout from the window-layout.json file
    WindowLayout loadWindowLayoutFromFile() const;

    // Apply a window layout for this window manager.
    void applyWindowLayout(const WindowLayout &layout);

    // Contains the full path to the window layout file, e.g. /home/pajlada/.local/share/Chatterino/Settings/window-layout.json
    const QString windowLayoutFilePath;

    bool shuttingDown_ = false;

    QRect emotePopupBounds_;

    std::atomic<int> generation_{0};

    std::vector<Window *> windows_;

    std::unique_ptr<FramelessEmbedWindow> framelessEmbedWindow_;
    Window *mainWindow_{};
    Window *selectedWindow_{};

    MessageElementFlags wordFlags_{};

    QTimer *saveTimer;

    pajlada::Signals::SignalHolder signalHolder;

    SignalListener updateWordTypeMaskListener;
    SignalListener forceLayoutChannelViewsListener;
    SignalListener layoutChannelViewsListener;
    SignalListener invalidateChannelViewBuffersListener;
    SignalListener repaintVisibleChatWidgetsListener;

    friend class Window;  // this is for selectedWindow_
};

}  // namespace chatterino
