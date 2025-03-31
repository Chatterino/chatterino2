#include "widgets/listview/GenericListView.hpp"

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

            this->requestClose();
        });
}

void GenericListView::setModel(QAbstractItemModel *model)
{
    auto *casted = dynamic_cast<GenericListModel *>(model);
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

bool GenericListView::eventFilter(QObject * /*watched*/, QEvent *event)
{
    if (this->model_ == nullptr)
    {
        return false;
    }

    if (event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        int key = keyEvent->key();

        if (key == Qt::Key_Enter || key == Qt::Key_Return)
        {
            this->acceptCompletion();
            return true;
        }

        if (key == Qt::Key_Tab)
        {
            if (this->invokeActionOnTab_)
            {
                this->acceptCompletion();
            }
            else
            {
                this->focusNextCompletion();
            }

            return true;
        }

        if (key == Qt::Key_Backtab && !this->invokeActionOnTab_)
        {
            this->focusPreviousCompletion();
            return true;
        }

        if (key == Qt::Key_Down)
        {
            this->focusNextCompletion();
            return true;
        }

        if (key == Qt::Key_Up)
        {
            this->focusPreviousCompletion();
            return true;
        }

        if (key == Qt::Key_Escape)
        {
            this->requestClose();
            return true;
        }
    }

    return false;
}

void GenericListView::refreshTheme(const Theme &theme)
{
    const auto textCol = theme.window.text.name(QColor::HexArgb);

    auto accentColor = theme.accent;
    accentColor.setAlpha(100);
    const auto selCol = accentColor.name(QColor::HexArgb);

    const auto listStyle = QStringLiteral(R"(
        QListView {
            border: none;
            color: %1;
            background: transparent;
            selection-background-color: %2;
        }

        QAbstractScrollArea::corner {
            border: none;
        }

        QScrollBar {
            background: transparent;
        }
        QScrollBar:vertical {
            margin-left: 4;
        }
        QScrollBar:horizontal {
            margin-top: 4;
        }

        QScrollBar::add-line,
        QScrollBar::sub-line,
        QScrollBar::left-arrow,
        QScrollBar::right-arrow,
        QScrollBar::down-arrow,
        QScrollBar::up-arrow {
            width: 0;
            height: 0;
        }

        QScrollBar::handle {
            background: %2;
            border-radius: 4;
        }
        QScrollBar::handle:vertical {
            min-height: 8;
            width: 8;
        }
        QScrollBar::handle:horizontal {
            min-width: 8;
            height: 8;
        })")
                               .arg(textCol, selCol);

    this->setStyleSheet(listStyle);
}

bool GenericListView::acceptCompletion()
{
    const QModelIndex &curIdx = this->currentIndex();
    const int count = this->model_->rowCount(curIdx);
    if (count <= 0)
    {
        return false;
    }

    const auto index = this->currentIndex();
    auto *item = GenericListItem::fromVariant(index.data());

    item->action();

    this->requestClose();

    return true;
}

void GenericListView::focusNextCompletion()
{
    const QModelIndex &curIdx = this->currentIndex();
    const int curRow = curIdx.row();
    const int count = this->model_->rowCount(curIdx);
    if (count <= 0)
    {
        return;
    }

    const int newRow = (curRow + 1) % count;

    this->setCurrentIndex(curIdx.siblingAtRow(newRow));
}

void GenericListView::focusPreviousCompletion()
{
    const QModelIndex &curIdx = this->currentIndex();
    const int curRow = curIdx.row();
    const int count = this->model_->rowCount(curIdx);
    if (count <= 0)
    {
        return;
    }

    int newRow = curRow - 1;
    if (newRow < 0)
    {
        newRow += count;
    }

    this->setCurrentIndex(curIdx.siblingAtRow(newRow));
}

void GenericListView::requestClose()
{
    this->closeRequested();
}

}  // namespace chatterino
