#include "widgets/helper/RegExpItemDelegate.hpp"

#include <QLineEdit>

namespace chatterino {

RegExpItemDelegate::RegExpItemDelegate(QObject *parent,
                                       QRegularExpression regexp)
    : QStyledItemDelegate(parent)
    , regexp_(regexp)
{
}

QWidget *RegExpItemDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    auto *editor = new QLineEdit(parent);
    editor->setValidator(
        new QRegularExpressionValidator(this->regexp_, editor));
    return editor;
}

}  // namespace chatterino
