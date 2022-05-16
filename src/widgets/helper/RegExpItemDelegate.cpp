#include "widgets/helper/RegExpItemDelegate.hpp"

#include "widgets/helper/TrimRegExpValidator.hpp"

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
    editor->setValidator(new TrimRegExpValidator(this->regexp_, editor));
    return editor;
}

}  // namespace chatterino
