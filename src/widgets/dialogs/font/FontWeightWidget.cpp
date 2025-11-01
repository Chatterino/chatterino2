#include "widgets/dialogs/font/FontWeightWidget.hpp"

#include "widgets/dialogs/font/IntItem.hpp"

#include <QBoxLayout>
#include <QFontDatabase>
#include <QLabel>
#include <QList>
#include <QString>
#include <QStringList>

namespace chatterino {

namespace {

/// Get a list of weights available for the given font family
QList<int> getWeights(const QString &family)
{
    QList<int> weights;
    QStringList styles = QFontDatabase::styles(family);

    for (const QString &style : styles)
    {
        int weight = QFontDatabase::weight(family, style);
        if (!weights.contains(weight))
        {
            weights.append(weight);
        }
    }

    return weights;
}

}  // namespace

FontWeightWidget::FontWeightWidget(const QFont &startFont, QWidget *parent)
    : QWidget(parent)
    , list(new QListWidget)
{
    auto *layout = new QVBoxLayout;

    this->setLayout(layout);

    this->setFamily(startFont.family());
    if (IntItem *item = findIntItemInList(this->list, startFont.weight()))
    {
        this->list->setCurrentItem(item);
    }

    layout->addWidget(new QLabel("Weight"));
    layout->addWidget(this->list);
    layout->setContentsMargins(0, 0, 0, 0);

    QObject::connect(this->list, &QListWidget::currentRowChanged, this,
                     [this](int row) {
                         if (row >= 0)
                         {
                             Q_EMIT this->selectedChanged();
                         }
                     });
}

void FontWeightWidget::setFamily(const QString &family)
{
    QSignalBlocker listSignalBlocker{this->list};

    QList<int> weights = getWeights(family);
    int currentCount = this->list->count();
    int newCount = static_cast<int>(weights.count());
    int leastCount = std::min(currentCount, newCount);

    std::ranges::sort(weights);

    for (int i = 0; i < leastCount; ++i)
    {
        auto *cast = dynamic_cast<IntItem *>(this->list->item(i));
        assert(cast);
        cast->setValue(weights[i]);
        cast->setHidden(false);
    }
    for (int i = currentCount; i < newCount; ++i)
    {
        this->list->addItem(new IntItem(weights[i]));
    }
    for (int i = newCount; i < currentCount; ++i)
    {
        this->list->item(i)->setHidden(true);
    }

    int startRow = 0;
    for (int closest = INT_MAX, i = 0; i < weights.count(); ++i)
    {
        int diff = std::abs(weights[i] - QFont::Normal);
        if (diff < closest)
        {
            closest = diff;
            startRow = i;
        }
    }

    this->list->setCurrentRow(startRow);
}

int FontWeightWidget::getSelected() const
{
    auto *cast = dynamic_cast<IntItem *>(this->list->currentItem());
    return cast ? cast->getValue() : -1;
}

}  // namespace chatterino
