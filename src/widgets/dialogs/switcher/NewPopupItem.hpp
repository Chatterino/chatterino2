#pragma once

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

namespace chatterino {

class NewPopupItem : public AbstractSwitcherItem
{
public:
    /**
     * @brief   Construct a new NewPopupItem that opens a passed channel in a new
     *          popup.
     *
     * @param   channelName name of channel to open
     */
    NewPopupItem(const QString &channelName);

    /**
     * @brief   Open the channel passed in the constructor in a new popup.
     */
    void action() override;

    void paint(QPainter *painter, const QRect &rect) const override;
    QSize sizeHint(const QRect &rect) const override;

private:
    static constexpr const char *TEXT_FORMAT =
        "Open channel \"%1\" in new popup";
    QString channelName_;
    QString text_;
};

}  // namespace chatterino
