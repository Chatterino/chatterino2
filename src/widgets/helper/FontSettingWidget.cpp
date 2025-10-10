#include "widgets/helper/FontSettingWidget.hpp"

#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStringListModel>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextEdit>
#include <QVBoxLayout>

namespace chatterino {

namespace {

class IntItem : public QListWidgetItem
{
    int value;

public:
    void setText(QString const &) = delete;

    IntItem(int v = 0)
        : QListWidgetItem(QString::number(v))
        , value(v)
    {
    }

    bool operator<(QListWidgetItem const &other) const override
    {
        auto const *cast = dynamic_cast<const IntItem *>(&other);
        assert(cast);
        return this->value < cast->value;
    }

    void setValue(int v)
    {
        this->value = v;
        this->QListWidgetItem::setText(QString::number(v));
    }

    int getValue() const
    {
        return this->value;
    }
};

IntItem *findIntItemInList(QListWidget *list, int value)
{
    int n = list->count();
    int i = 0;
    for (; i < n; ++i)
    {
        auto *item = dynamic_cast<IntItem *>(list->item(i));
        assert(item);

        if (item->getValue() == value)
        {
            return item;
        }
    }

    return nullptr;
}

class FontSizeWidget : public QWidget
{
    Q_OBJECT

    IntItem *customItem = new IntItem;
    QListWidget *list = new QListWidget;
    QSpinBox *edit = new QSpinBox;

    void setListTo(int size)
    {
        if (IntItem *item = findIntItemInList(this->list, size))
        {
            this->customItem->setHidden(item != customItem);
            this->list->setCurrentItem(item);
            return;
        }

        this->customItem->setValue(size);
        this->customItem->setHidden(false);
        this->list->setCurrentItem(this->customItem);
    }

public:
    FontSizeWidget(QFont const &initialFont, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout;
        auto *header = new QHBoxLayout;

        this->setLayout(layout);
        this->list->setSortingEnabled(true);
        this->list->addItem(this->customItem);
        this->customItem->setHidden(true);

        for (int size : QFontDatabase::standardSizes())
        {
            this->list->addItem(new IntItem(size));
        }

        layout->addLayout(header);
        layout->addWidget(this->list);
        layout->setContentsMargins(0, 0, 0, 0);

        header->addWidget(new QLabel("Size"));
        header->addWidget(this->edit);
        header->setContentsMargins(0, 0, 0, 0);

        this->edit->setValue(initialFont.pointSize());
        this->setListTo(initialFont.pointSize());

        QObject::connect(this->edit, &QSpinBox::valueChanged, this,
                         [this](int value) {
                             this->list->blockSignals(true);
                             this->setListTo(value);
                             this->list->blockSignals(false);
                             Q_EMIT this->selectedChanged();
                         });

        QObject::connect(this->list, &QListWidget::currentItemChanged, this,
                         [this](QListWidgetItem *item) {
                             auto *cast = dynamic_cast<IntItem *>(item);
                             assert(cast);
                             this->edit->blockSignals(true);
                             this->edit->setValue(cast->getValue());
                             this->edit->blockSignals(false);
                             Q_EMIT this->selectedChanged();
                         });
    }

    int getSelected() const
    {
        auto *item = dynamic_cast<IntItem *>(this->list->currentItem());
        return item ? item->getValue() : -1;
    }

Q_SIGNALS:
    void selectedChanged();
};

class FontFamiliesWidget : public QWidget
{
    Q_OBJECT

    QListView *list = new QListView;
    QStringListModel *model =
        new QStringListModel(QFontDatabase::families(), this);
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);

public:
    FontFamiliesWidget(QFont const &initialFont, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout;
        auto *header = new QHBoxLayout;
        auto *search = new QLineEdit;

        this->setLayout(layout);

        this->list->setModel(this->proxy);
        this->list->setEditTriggers(QAbstractItemView::NoEditTriggers);

        this->proxy->setSourceModel(this->model);
        this->proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

        layout->addLayout(header);
        layout->addWidget(this->list);
        layout->setContentsMargins(0, 0, 0, 0);

        header->addWidget(new QLabel("Font"));
        header->addWidget(search);
        header->setContentsMargins(0, 0, 0, 0);

        search->setPlaceholderText("Search...");

        QObject::connect(search, &QLineEdit::textChanged, this->proxy,
                         &QSortFilterProxyModel::setFilterFixedString);

        QObject::connect(
            this->list->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](QModelIndex const &modelIndex, QModelIndex const &) {
                if (modelIndex.isValid())
                {
                    Q_EMIT selectedChanged();
                }
            });

        this->setSelected(initialFont.family());
    }

    QString getSelected()
    {
        QModelIndex proxyIndex = this->list->currentIndex();

        if (!proxyIndex.isValid())
        {
            return {};
        }

        QModelIndex modelIndex = this->proxy->mapToSource(proxyIndex);
        return this->model->data(modelIndex).toString();
    }

    void setSelected(QString const &family)
    {
        qsizetype row = this->model->stringList().indexOf(family);

        if (row < 0)
        {
            return;
        }

        QModelIndex modelIndex = this->model->index(static_cast<int>(row));
        QModelIndex proxyIndex = this->proxy->mapFromSource(modelIndex);

        this->list->selectionModel()->setCurrentIndex(
            proxyIndex, QItemSelectionModel::ClearAndSelect);
    }

Q_SIGNALS:
    void selectedChanged();
};

class FontWeightWidget : public QWidget
{
    Q_OBJECT

    QListWidget *list = new QListWidget;

public:
    FontWeightWidget(QFont const &initialFont, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout;

        this->setLayout(layout);
        this->list->setSortingEnabled(true);

        layout->addWidget(new QLabel("Weight"));
        layout->addWidget(list);
        layout->setContentsMargins(0, 0, 0, 0);

        QObject::connect(list, &QListWidget::currentRowChanged, this,
                         [this](int row) {
                             if (row >= 0)
                             {
                                 Q_EMIT this->selectedChanged();
                             }
                         });

        this->setFamily(initialFont.family());

        if (IntItem *item = findIntItemInList(this->list, initialFont.weight()))
        {
            this->list->setCurrentItem(item);
        }
    }

    void setFamily(QString const &family)
    {
        QSet<int> weights;
        int defaultWeight = QFont(family).weight();
        auto *defaultItem = new IntItem(defaultWeight);

        this->list->clear();
        this->list->addItem(defaultItem);

        weights.insert(defaultWeight);

        for (auto const &style : QFontDatabase::styles(family))
        {
            int weight = QFontDatabase::weight(family, style);

            if (!weights.contains(weight))
            {
                weights.insert(weight);
                this->list->addItem(new IntItem(weight));
            }
        }

        this->list->setCurrentItem(defaultItem);
    }

    int getSelected() const
    {
        auto *cast = dynamic_cast<IntItem *>(this->list->currentItem());
        return cast ? cast->getValue() : -1;
    }

Q_SIGNALS:
    void selectedChanged();
};

class FontDialog : public QDialog
{
    Q_OBJECT

    QTextEdit *sampleBox = new QTextEdit;

    FontFamiliesWidget *fontFamiliesW;
    FontSizeWidget *fontSizeW;
    FontWeightWidget *fontWeightW;

    void updateSampleFont()
    {
        QTextCursor cursor = this->sampleBox->textCursor();
        QTextCharFormat format;

        cursor.select(QTextCursor::Document);
        format.setFont(this->getSelected());
        cursor.setCharFormat(format);
    }

public:
    FontDialog(QFont const &initialFont, QWidget *parent = nullptr)
        : QDialog(parent)
        , fontFamiliesW(new FontFamiliesWidget(initialFont))
        , fontSizeW(new FontSizeWidget(initialFont))
        , fontWeightW(new FontWeightWidget(initialFont))
    {
        auto *layout = new QVBoxLayout;
        auto *choiceLayout = new QHBoxLayout;
        auto *choiceSideLayout = new QVBoxLayout;
        auto *buttons = new QDialogButtonBox;
        auto *confirmBtn = new QPushButton("Ok");
        auto *cancelBtn = new QPushButton("Cancel");

        this->setWindowTitle("Pick Font");
        this->setLayout(layout);
        this->resize(450, 450);
        this->sampleBox->setAcceptRichText(false);
        this->sampleBox->setText("The quick brown fox jumps over the lazy dog");

        layout->addLayout(choiceLayout, 5);
        layout->addWidget(new QLabel("Sample"));
        layout->addWidget(this->sampleBox, 1);
        layout->addWidget(buttons);

        choiceLayout->addWidget(this->fontFamiliesW, 5);
        choiceLayout->addLayout(choiceSideLayout, 3);

        choiceSideLayout->addWidget(this->fontSizeW);
        choiceSideLayout->addWidget(this->fontWeightW);

        buttons->addButton(confirmBtn, QDialogButtonBox::AcceptRole);
        buttons->addButton(cancelBtn, QDialogButtonBox::RejectRole);

        QObject::connect(buttons, &QDialogButtonBox::accepted, this,
                         &QDialog::accept);

        QObject::connect(buttons, &QDialogButtonBox::rejected, this,
                         &QDialog::reject);

        QObject::connect(this->fontFamiliesW,
                         &FontFamiliesWidget::selectedChanged, this, [this]() {
                             this->fontWeightW->setFamily(
                                 this->fontFamiliesW->getSelected());
                             this->updateSampleFont();
                         });

        QObject::connect(this->fontWeightW, &FontWeightWidget::selectedChanged,
                         this, [this]() {
                             this->updateSampleFont();
                         });

        QObject::connect(this->fontSizeW, &FontSizeWidget::selectedChanged,
                         this, [this]() {
                             this->updateSampleFont();
                         });

        this->updateSampleFont();
    }

    QFont getSelected()
    {
        auto const &family = this->fontFamiliesW->getSelected();
        auto pointSize = this->fontSizeW->getSelected();
        auto weight = this->fontWeightW->getSelected();
        return {family, pointSize, weight};
    }
};

}  // namespace

void FontSettingWidget::updateCurrentLabel()
{
    QFont font = getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1);
    auto family = font.family();
    auto ptSize = QString::number(font.pointSize());
    this->currentLabel->setText(family + ", " + ptSize + "pt");
}

FontSettingWidget::FontSettingWidget(QWidget *parent)
    : QWidget(parent)
    , currentLabel(new QLabel)
{
    auto *button = new QPushButton;

    button->setIcon(QIcon(":/buttons/edit.svg"));

    QObject::connect(button, &QPushButton::clicked, this, [this]() {
        FontDialog dialog(
            getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1), this);

        if (dialog.exec() != QDialog::Accepted)
        {
            return;
        }

        QFont font = dialog.getSelected();
        getSettings()->chatFontFamily = font.family();
        getSettings()->chatFontSize = font.pointSize();
        getSettings()->chatFontWeight = font.weight();

        this->updateCurrentLabel();
    });

    auto *layout = new QHBoxLayout;

    layout->addWidget(new QLabel("Font:"));
    layout->addStretch(1);
    layout->addWidget(this->currentLabel);
    layout->addWidget(button);
    layout->setContentsMargins(0, 0, 0, 0);

    this->updateCurrentLabel();
    this->setLayout(layout);
}

}  // namespace chatterino

#include "FontSettingWidget.moc"
