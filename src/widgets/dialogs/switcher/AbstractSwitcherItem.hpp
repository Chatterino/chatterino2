#pragma once

namespace chatterino {

class AbstractSwitcherItem
{
public:
    /**
     * Attempt to obtain an AbstractSwitcherItem * from the passed QVariant.
     *
     * @param variant   variant to try to convert to AbstractSwitcherItem *
     *
     * @return an AbstractSwitcherItem * if the QVariant could be converted,
     *         or nullptr if the variant did not contain AbstractSwitcherItem *
     */
    static AbstractSwitcherItem *fromVariant(const QVariant &variant);

    virtual ~AbstractSwitcherItem() = default;

    AbstractSwitcherItem(const QIcon &icon);

    virtual void action() = 0;

    virtual void paint(QPainter *painter, const QRect &rect) const = 0;
    virtual QSize sizeHint(const QRect &rect) const = 0;

protected:
    QIcon icon_;
    static const QSize ICON_SIZE;
};

}  // namespace chatterino

Q_DECLARE_METATYPE(chatterino::AbstractSwitcherItem *);
