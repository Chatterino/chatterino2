#include "GenericListView.hpp"
#include "singletons/Theme.hpp"
#include "widgets/listview/GenericListModel.hpp"

#include <QKeyEvent>

namespace chatterino {

GenericListView::GenericListView()
    : itemDelegate_(this)
{
    this->setSelectionMode(QAbstractItemView::SingleSelection);
    this->setSelectionBehavior(QAbstractItemView::SelectItems);
    this->setItemDelegate(&this->itemDelegate_);

    QObject::connect(
        this, &QListView::clicked, this, [this](const QModelIndex &index) {
            auto *item = GenericListItem::fromVariant(index.data());
            item->action();

            emit this->closeRequested();
        });
}

void GenericListView::setModel(QAbstractItemModel *model)
{
    auto casted = dynamic_cast<GenericListModel *>(model);
    assert(casted);
    this->setModel(casted);
}

void GenericListView::setModel(GenericListModel *model)
{
    this->model_ = model;
    QListView::setModel(model);
}

void GenericListView::setInvokeActionOnTab(bool value)
{
    this->invokeActionOnTab_ = value;
}

void GenericListView::tryAcceptCompletion(bool wasTabPressed)
{
    if (wasTabPressed && !this->invokeActionOnTab_)
    {
        return this->focusNextCompletion();
    }
    const QModelIndex &curIdx = this->currentIndex();
    const int curRow = curIdx.row();
    const int count = this->model_->rowCount(curIdx);
    // keep this before the other tab handler
    if (count <= 0)
        return;

    const auto index = this->currentIndex();
    auto *item = GenericListItem::fromVariant(index.data());

    item->action();

    emit this->closeRequested();
    return;
}
void GenericListView::focusNextCompletion()
{
    const QModelIndex &curIdx = this->currentIndex();
    const int curRow = curIdx.row();
    const int count = this->model_->rowCount(curIdx);
    if (count <= 0)
        return;

    const int newRow = (curRow + 1) % count;

    this->setCurrentIndex(curIdx.siblingAtRow(newRow));
    return;
}
void GenericListView::focusPreviousCompletion()
{
    const QModelIndex &curIdx = this->currentIndex();
    const int curRow = curIdx.row();
    const int count = this->model_->rowCount(curIdx);
    if (count <= 0)
        return;

    int newRow = curRow - 1;
    if (newRow < 0)
        newRow += count;

    this->setCurrentIndex(curIdx.siblingAtRow(newRow));
    return;
}

void GenericListView::requestClose()
{
    emit this->closeRequested();
}

// TODO(mm2pl): probably remove this
bool GenericListView::eventFilter(QObject * /*watched*/, QEvent *event)
{
    if (!this->model_)
        return false;

    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();

        if (key == Qt::Key_Enter || key == Qt::Key_Return || key == Qt::Key_Tab)
        {
            // keep this before the other tab handler
            this->tryAcceptCompletion(key == Qt::Key_Tab);
        }
        else if (key == Qt::Key_Down || key == Qt::Key_Tab)
        {
            this->focusNextCompletion();
        }
        else if (key == Qt::Key_Up ||
                 (!this->invokeActionOnTab_ && key == Qt::Key_Backtab))
        {
            this->focusPreviousCompletion();
        }
        else if (key == Qt::Key_Escape)
        {
            this->requestClose();
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

void GenericListView::refreshTheme(const Theme &theme)
{
    const QString textCol = theme.window.text.name();
    const QString bgCol = theme.window.background.name();

    const QString selCol =
        (theme.isLightTheme()
             ? "#68B1FF"  // Copied from Theme::splits.input.styleSheet
             : theme.tabs.selected.backgrounds.regular.color().name());

    const QString listStyle =
        QString(
            "color: %1; background-color: %2; selection-background-color: %3")
            .arg(textCol)
            .arg(bgCol)
            .arg(selCol);

    this->setStyleSheet(listStyle);
}

}  // namespace chatterino
