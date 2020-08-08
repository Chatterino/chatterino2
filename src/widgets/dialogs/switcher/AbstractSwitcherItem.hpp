#pragma once

namespace chatterino {

class AbstractSwitcherItem
{
public:
    /**
     * @brief   Attempt to obtain an AbstractSwitcherItem * from the passed QVariant.
     *
     * @param   variant variant to try to convert to AbstractSwitcherItem *
     *
     * @return  an AbstractSwitcherItem * if the QVariant could be converted,
     *          or nullptr if the variant did not contain AbstractSwitcherItem *
     */
    static AbstractSwitcherItem *fromVariant(const QVariant &variant);

    virtual ~AbstractSwitcherItem() = default;

    /**
     * @brief   Since all switcher items are required to have an icon, we require it
     *          in the base class constructor.
     *
     * @param   icon    icon to be displayed in the switcher list
     */
    AbstractSwitcherItem(const QIcon &icon);

    /**
     * @brief   Action to perform when this item is activated. Must be implemented in
     *          subclasses.
     */
    virtual void action() = 0;

    virtual void paint(QPainter *painter, const QRect &rect) const = 0;
    virtual QSize sizeHint(const QRect &rect) const = 0;

protected:
    QIcon icon_;
    static const QSize ICON_SIZE;
};

}  // namespace chatterino

// This allows us to store AbstractSwitcherItem * as a QVariant
Q_DECLARE_METATYPE(chatterino::AbstractSwitcherItem *);
