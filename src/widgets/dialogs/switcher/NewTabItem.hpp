#pragma once

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

namespace chatterino {

class Window;

class NewTabItem : public AbstractSwitcherItem
{
public:
    /**
     * @brief   Construct a new NewTabItem that opens a passed channel in a new
     *          tab.
     *
     * @param   channelName name of channel to open
     */
    NewTabItem(Window *window_, const QString &channelName);

    /**
     * @brief   Open the channel passed in the constructor in a new tab.
     */
    void action() override;

    void paint(QPainter *painter, const QRect &rect) const override;
    QSize sizeHint(const QRect &rect) const override;

private:
    static constexpr const char *TEXT_FORMAT = "Open channel \"%1\" in new tab";
    QString channelName_;
    QString text_;
    Window *window{};
};

}  // namespace chatterino
