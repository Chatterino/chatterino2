#pragma once

#include <qpushbutton.h>
#include <QSize>
#include <QStyledItemDelegate>

namespace chatterino {

class TestDelegate : public QStyledItemDelegate
{
public:
    explicit TestDelegate(QObject *parent);
    ~TestDelegate();

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    void setEditorData(QWidget *editor,
                       const QModelIndex &index) const override;

    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) override;

private:
    QPushButton *btn;
};

}  // namespace chatterino
