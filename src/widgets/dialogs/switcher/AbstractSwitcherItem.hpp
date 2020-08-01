#pragma once

namespace chatterino {

class AbstractSwitcherItem
{
public:
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
