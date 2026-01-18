#include "widgets/settingspages/HighlightsBetaConfigureDelegate.hpp"

#include "singletons/Resources.hpp"
#include "widgets/helper/color/Checkerboard.hpp"

#include <qapplication.h>
#include <qstyleoption.h>

namespace chatterino {

HighlightsBetaConfigureDelegate::HighlightsBetaConfigureDelegate(
    QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void HighlightsBetaConfigureDelegate::paint(QPainter *painter,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &index) const
{
    // QStyledItemDelegate::paint(painter, option, index);

    QStyleOptionButton xd;
    xd.state = option.state;

    // if (option.state)
    xd.features = QStyleOptionButton::None;
    xd.icon = getResources().buttons.edit;
    xd.text = "Edit...";
    auto rect = option.rect;
    // TODO: this looks a bit ugly
    rect.setSize(rect.size() * 0.8);
    auto iconSize = QSize{rect.height(), rect.height()};
    xd.iconSize = iconSize * 0.9;
    xd.rect = rect;

    QApplication::style()->drawControl(QStyle::CE_PushButton, &xd, painter,
                                       nullptr);
}

}  // namespace chatterino
