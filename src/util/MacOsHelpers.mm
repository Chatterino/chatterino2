#include "util/MacOsHelpers.h"

#include <AppKit/AppKit.h>
#include <QUrl>
#include <QWidget>

void chatterinoSetMacOsActivationPolicyProhibited()
{
    [[NSApplication sharedApplication] setActivationPolicy:NSApplicationActivationPolicyProhibited];
}


namespace chatterino {

#ifdef Q_OS_DARWIN
void setMacOSWindowManaged(QWidget &widget)
{
    if (widget.windowType() != Qt::Window && widget.windowType() != Qt::Dialog)
    {
        return;
    }

    auto *view = reinterpret_cast<NSView *>(widget.effectiveWinId());
    auto *window = view.window;
    if (window == nil)
    {
        return;
    }

    window.collectionBehavior =
        (window.collectionBehavior & ~(NSWindowCollectionBehaviorTransient |
                                       NSWindowCollectionBehaviorStationary)) |
        NSWindowCollectionBehaviorManaged;
}

QString getMacOSDefaultBrowserPath()
{
    @autoreleasepool {
        NSURL *httpUrl = [NSURL URLWithString:@"https://"];
        NSURL *appUrl = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:httpUrl];
        if (appUrl == nil)
        {
            return {};
        }

        NSBundle *bundle = [NSBundle bundleWithURL:appUrl];
        NSURL *execUrl = bundle.executableURL;
        if (execUrl == nil)
        {
            return {};
        }

        return QUrl::fromNSURL(execUrl).toLocalFile();    }
}
#endif

}  // namespace chatterino
