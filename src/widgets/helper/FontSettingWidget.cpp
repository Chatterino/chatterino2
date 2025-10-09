#include "widgets/helper/FontSettingWidget.hpp"

#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"

#include <QDebug>
#include <QDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStringListModel>
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
        QListWidgetItem::setText(QString::number(v));
    }

    int getValue() const
    {
        return this->value;
    }
};

class FontSizeWidget : public QWidget
{
    Q_OBJECT

    IntItem *customItem = new IntItem;
    QListWidget *list = new QListWidget;

public:
    FontSizeWidget(QFont const &initialFont, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout;
        auto *header = new QHBoxLayout;
        auto *edit = new QSpinBox;

        this->setLayout(layout);
        this->customItem->setHidden(true);
        this->list->setSortingEnabled(true);
        this->list->addItem(this->customItem);

        for (int size : QFontDatabase::standardSizes())
        {
            this->list->addItem(new IntItem(size));
        }

        layout->addLayout(header);
        layout->addWidget(this->list);
        layout->setContentsMargins(0, 0, 0, 0);

        header->addWidget(new QLabel("Size"));
        header->addWidget(edit);
        header->setContentsMargins(0, 0, 0, 0);

        QObject::connect(edit, &QSpinBox::valueChanged, this,
                         [this](int value) {
                             this->setSelected(value);
                         });

        QObject::connect(this->list, &QListWidget::currentItemChanged, this,
                         [this, edit](QListWidgetItem *item) {
                             if (item == this->customItem)
                             {
                                 return;
                             }

                             auto *cast = dynamic_cast<IntItem *>(item);
                             assert(cast);

                             this->customItem->setHidden(true);
                             edit->setValue(cast->getValue());
                             Q_EMIT this->selectedChanged();
                         });

        this->setSelected(initialFont.pointSize());
    }

    void setSelected(int value)
    {
        int n = this->list->count();
        int i = 0;

        for (; i < n; ++i)
        {
            auto *item = dynamic_cast<IntItem *>(this->list->item(i));
            assert(item);

            if (item->getValue() == value)
            {
                this->list->setCurrentItem(item);
                this->customItem->setHidden(true);
                return;
            }
        }

        this->customItem->setValue(value);
        this->customItem->setHidden(false);
        this->list->setCurrentItem(this->customItem);
    }

    int getSelected() const
    {
        auto *item = dynamic_cast<IntItem *>(this->list->currentItem());

        if (!item)
        {
            return -1;
        }

        return item->getValue();
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

        layout->addLayout(header);
        layout->addWidget(this->list);
        layout->setContentsMargins(0, 0, 0, 0);

        header->addWidget(new QLabel("Font"));
        header->addWidget(search);
        header->setContentsMargins(0, 0, 0, 0);

        search->setPlaceholderText("Search...");

        this->list->setModel(this->proxy);
        this->list->setEditTriggers(QAbstractItemView::NoEditTriggers);

        this->proxy->setSourceModel(this->model);
        this->proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

        QObject::connect(search, &QLineEdit::textChanged, this->proxy,
                         &QSortFilterProxyModel::setFilterFixedString);

        QObject::connect(
            this->list->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](QModelIndex const &modelIndex, QModelIndex const &) {
                if (!modelIndex.isValid())
                {
                    return;
                }
                Q_EMIT selectedChanged();
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

class FontStylesWidget : public QWidget
{
    Q_OBJECT

    QListWidget *list = new QListWidget;
    int currentWeight = 0;

public:
    FontStylesWidget(QFont const &initialFont, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        auto *layout = new QVBoxLayout;

        layout->addWidget(new QLabel("Weight"));
        layout->addWidget(list);
        layout->setContentsMargins(0, 0, 0, 0);

        this->list->setSortingEnabled(true);

        QObject::connect(
            list, &QListWidget::currentRowChanged, this, [this](int row) {
                if (row < 0)
                {
                    return;
                }

                auto *cast = dynamic_cast<IntItem *>(this->list->item(row));
                assert(cast);
                this->currentWeight = cast->getValue();

                Q_EMIT this->selectedChanged();
            });

        this->setLayout(layout);
        this->setFamily(initialFont.family());
    }

    void setFamily(QString const &family)
    {
        QSet<int> weights;

        this->list->clear();

        int defaultWeight = QFont(family).weight();
        auto *defaultItem = new IntItem(defaultWeight);

        weights.insert(defaultWeight);

        this->list->addItem(defaultItem);

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
        return this->currentWeight;
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
    FontStylesWidget *fontWeightW;

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
        , fontWeightW(new FontStylesWidget(initialFont))
    {
        this->setWindowTitle("Pick Font");
        this->sampleBox->setAcceptRichText(false);
        this->sampleBox->setText("The quick brown fox jumps over the lazy dog");

        this->resize(400, 200);

        auto *layout = new QVBoxLayout;
        auto *choiceLayout = new QHBoxLayout;
        auto *choiceLayout2 = new QVBoxLayout;
        auto *buttons = new QDialogButtonBox;
        auto *confirmBtn = new QPushButton("Ok");
        auto *cancelBtn = new QPushButton("Cancel");

        layout->addLayout(choiceLayout, 2);
        layout->addWidget(new QLabel("Sample"));
        layout->addWidget(this->sampleBox, 1);
        layout->addWidget(buttons);

        choiceLayout->addWidget(this->fontFamiliesW, 3);
        choiceLayout->addLayout(choiceLayout2, 1);

        choiceLayout2->addWidget(this->fontSizeW);
        choiceLayout2->addWidget(this->fontWeightW);

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

        QObject::connect(this->fontWeightW, &FontStylesWidget::selectedChanged,
                         this, [this]() {
                             this->updateSampleFont();
                         });

        QObject::connect(this->fontSizeW, &FontSizeWidget::selectedChanged,
                         this, [this]() {
                             this->updateSampleFont();
                         });
        this->updateSampleFont();
        this->setLayout(layout);
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
    });

    auto *layout = new QHBoxLayout;

    layout->addWidget(new QLabel("Font:"));
    layout->addStretch(1);
    layout->addWidget(this->currentLabel);
    layout->addWidget(button);
    layout->setContentsMargins(0, 0, 0, 0);

    this->listener.addSetting(getSettings()->chatFontFamily);
    this->listener.addSetting(getSettings()->chatFontSize);
    this->listener.addSetting(getSettings()->chatFontWeight);
    this->listener.setCB([this] {
        this->updateCurrentLabel();
    });

    this->updateCurrentLabel();
    this->setLayout(layout);
}

}  // namespace chatterino

#include "FontSettingWidget.moc"
