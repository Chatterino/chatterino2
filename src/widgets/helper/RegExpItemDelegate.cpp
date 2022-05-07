#include "widgets/helper/RegExpItemDelegate.hpp"

namespace chatterino {

RegExpItemDelegate::RegExpItemDelegate(QObject *parent, QString pattern)
    : QStyledItemDelegate(parent)
    , pattern_(pattern)
{
}

QWidget *RegExpItemDelegate::createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    QLineEdit *editor = new QLineEdit(parent);
    QRegularExpression rx(this->pattern_);
    editor->setValidator(new QRegularExpressionValidator(rx, editor));
    return editor;
}

}  // namespace chatterino
