#include "TestDelegate.hpp"

#include "controllers/highlights/HighlightPhrase.hpp"
#include "widgets/helper/color/Checkerboard.hpp"

namespace chatterino {

TestDelegate::TestDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize TestDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    auto base = QStyledItemDelegate::sizeHint(option, index);

    qInfo() << "XXX: size hint when userrole+1 value is"
            << index.data(Qt::UserRole + 1).value<bool>();

    if (index.data(Qt::UserRole + 1).value<bool>())
    {
        base.setHeight(base.height() * 2);
    }
    else
    {
        // do nothing
    }

    return base;
}

void TestDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    auto data = index.data(Qt::DisplayRole);

    HighlightPhrase phrase("my phrase", true, false, true, true, true, "",
                           QColor{});

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (data.typeId() != QMetaType::QColor)
#else
    if (data.type() != QVariant::Color)
#endif
    {
        qInfo() << "XXX:" << data.typeId();
        qInfo() << "XXX:" << data.type();
        painter->save();
        painter->setBrush(Qt::white);
        painter->drawText(option.rect, data.value<QString>());
        painter->setBrush(QColor{255, 0, 255, 20});
        painter->setPen(QColor{255, 0, 255});
        painter->drawRect(option.rect);
        painter->restore();
        // return QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    auto color = data.value<QColor>();

    painter->save();
    if (color.alpha() != 255)
    {
        drawCheckerboard(*painter, option.rect,
                         std::min(option.rect.height() / 2, 10));
    }
    painter->setBrush(color);
    painter->drawRect(option.rect);
    painter->restore();
}

}  // namespace chatterino
