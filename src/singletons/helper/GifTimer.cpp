#include "singletons/helper/GifTimer.hpp"

#include "Application.hpp"
#include "singletons/Settings.hpp"

namespace chatterino
{
    void GIFTimer::initialize()
    {
#if 0
        this->timer.setInterval(30);

        getSettings()->animateEmotes.connect([this](bool enabled, auto) {
            if (enabled)
                this->timer.start();
            else
                this->timer.stop();
        });

        QObject::connect(&this->timer, &QTimer::timeout, [this] {
            if (getSettings()->animationsWhenFocused &&
                qApp->activeWindow() == nullptr)
                return;

            this->signal.invoke();
            getApp()->windows->repaintGifEmotes();
        });
#endif
    }
}  // namespace chatterino
