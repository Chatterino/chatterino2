#include "widgets/dialogs/font/FontSizeWidget.hpp"

#include "widgets/dialogs/font/IntItem.hpp"

#include <QBoxLayout>
#include <QFontDatabase>
#include <QLabel>
#include <QSignalBlocker>

namespace chatterino {

FontSizeWidget::FontSizeWidget(const QFont &startFont, QWidget *parent)
    : QWidget(parent)
    , customItem(new IntItem)
    , list(new QListWidget)
    , edit(new QSpinBox)
{
    auto *layout = new QVBoxLayout;
    auto *header = new QHBoxLayout;

    this->setLayout(layout);

    this->edit->setMinimum(1);
    this->list->setSortingEnabled(true);
    this->list->addItem(this->customItem);

    for (int size : QFontDatabase::standardSizes())
    {
        this->list->addItem(new IntItem(size));
    }

    this->edit->setValue(startFont.pointSize());
    this->setListSelected(startFont.pointSize());

    layout->addLayout(header);
    layout->addWidget(this->list);
    layout->setContentsMargins(0, 0, 0, 0);

    header->addWidget(new QLabel("Size"));
    header->addWidget(this->edit);

    QObject::connect(this->edit, &QSpinBox::valueChanged, this,
                     [this](int value) {
                         QSignalBlocker listSignalBlocker(this->list);
                         this->setListSelected(value);
                         Q_EMIT this->selectedChanged();
                     });

    QObject::connect(this->list, &QListWidget::currentItemChanged, this,
                     [this](QListWidgetItem *item) {
                         auto *cast = dynamic_cast<IntItem *>(item);
                         assert(cast);
                         QSignalBlocker editSignalBlocker(this->edit);
                         this->edit->setValue(cast->getValue());
                         Q_EMIT this->selectedChanged();
                     });
}

int FontSizeWidget::getSelected() const
{
    auto *item = dynamic_cast<IntItem *>(this->list->currentItem());
    return item ? item->getValue() : -1;
}

void FontSizeWidget::setListSelected(int size)
{
    if (IntItem *item = findIntItemInList(this->list, size))
    {
        this->customItem->setHidden(item != this->customItem);
        this->list->setCurrentItem(item);
        return;
    }

    this->customItem->setValue(size);
    this->customItem->setHidden(false);
    this->list->setCurrentItem(this->customItem);
}

}  // namespace chatterino
