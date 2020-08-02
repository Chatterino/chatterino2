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

    AbstractSwitcherItem(const QString &text);
    AbstractSwitcherItem(const QIcon &icon, const QString &text);

    virtual void action() = 0;

    void paint(QPainter *painter, const QRect &rect) const;
    QSize sizeHint(const QRect &rect) const;

private:
    QIcon icon_;
    QString text_;
};

}  // namespace chatterino

Q_DECLARE_METATYPE(chatterino::AbstractSwitcherItem *);
