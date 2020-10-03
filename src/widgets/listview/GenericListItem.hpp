#pragma once

namespace chatterino {

class GenericListItem
{
public:
    /**
     * @brief   Attempt to obtain an GenericListItem * from the passed QVariant.
     *
     * @param   variant variant to try to convert to GenericListItem *
     *
     * @return  an GenericListItem * if the QVariant could be converted,
     *          or nullptr if the variant did not contain GenericListItem *
     */
    static GenericListItem *fromVariant(const QVariant &variant);

    virtual ~GenericListItem() = default;

    GenericListItem();

    /**
     * @param   icon    icon to be displayed in the switcher list
     */
    GenericListItem(const QIcon &icon);

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

// This allows us to store GenericListItem * as a QVariant
Q_DECLARE_METATYPE(chatterino::GenericListItem *);
