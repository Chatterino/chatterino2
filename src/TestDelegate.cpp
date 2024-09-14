#include "TestDelegate.hpp"

#include "controllers/highlights/HighlightPhrase.hpp"
#include "TestWidget.hpp"
#include "widgets/helper/color/Checkerboard.hpp"

#include <QItemEditorFactory>
#include <qlistview.h>
#include <QPainterPath>
#include <qstyleditemdelegate.h>

namespace chatterino {

TestDelegate::TestDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
    auto *factory = new TestWidgetCreator();
    this->setItemEditorFactory(factory);
    this->btn = new QPushButton("xd");
}

TestDelegate::~TestDelegate()
{
    this->btn->deleteLater();
}

QSize TestDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    auto base = QStyledItemDelegate::sizeHint(option, index);

    if (index.data(Qt::UserRole + 1).value<bool>())
    {
        base.setHeight(base.height() * 5);
    }
    else
    {
        base.setHeight(base.height() * 5);
        // do nothing
    }

    return base;
}

void TestDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    auto data = index.data(Qt::DisplayRole);
    auto expanded = index.data(Qt::UserRole + 1).value<bool>();

    // auto *view = qobject_cast<QListView *>(this->parent());
    // view->openPersistentEditor(index);
    // if (view->isPersistentEditorOpen(index)) {
    // } else {
    //     return;
    // }

    // painter->begin(this->btn->paintEngine)
    // this->btn->paint()

    HighlightPhrase phrase("my phrase", true, false, true, true, true, "",
                           QColor{});

    painter->save();

    painter->setBrush(Qt::white);
    painter->drawText(option.rect, data.value<QString>());
    painter->setBrush(QColor{255, 0, 255, 20});
    painter->setPen(QColor{255, 0, 255});
    // painter->drawRect(option.rect);

    QPoint dist(10, 10);

    // painter->setPen(Qt::PenStyle::DotLine);

    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen;

    pen.setWidth(2);
    pen.setColor(QColor{255, 0, 255, 255});

    painter->setPen(pen);

    if (expanded)
    {
        QPainterPath path;
        path.moveTo(option.rect.topRight());
        path.lineTo(option.rect.topRight() - QPoint{10, -10});

        auto tr = QRect(option.rect.right() - 10, option.rect.top(), 5,
                        option.rect.height() / 2);

        painter->drawPath(path);
    }
    else
    {
        auto tr = QRect(option.rect.right() - 10, option.rect.top(), 5,
                        option.rect.height());
        painter->drawEllipse(tr);
    }

    painter->restore();
    // return QStyledItemDelegate::paint(painter, option, index);
}

void TestDelegate::setEditorData(QWidget *editor,
                                 const QModelIndex &index) const
{
    auto *realEditor = dynamic_cast<TestWidget *>(editor);
    realEditor->update(index.data().value<QString>());
    assert(realEditor);
}

bool TestDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index)
{
    auto *view = qobject_cast<QListView *>(this->parent());
    view->openPersistentEditor(index);
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

}  // namespace chatterino
