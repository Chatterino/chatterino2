#include "widgets/helper/FontSettingWidget.hpp"

#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStringListModel>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>

namespace chatterino {

namespace {

class IntItem : public QListWidgetItem
{
public:
    void setText(const QString &) = delete;

    IntItem(int v = 0, QListWidget *parent = nullptr)
        : QListWidgetItem(QString::number(v), parent, IntItem::typeId())
        , value(v)
    {
    }

    bool operator<(const QListWidgetItem &other) const override
    {
        const auto *cast = dynamic_cast<const IntItem *>(&other);
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

    static int typeId()
    {
        return QListWidgetItem::UserType + 123;
    }

private:
    int value;
};

IntItem *findIntItemInList(QListWidget *list, int value)
{
    int n = list->count();
    for (int i = 0; i < n; ++i)
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

public:
    FontSizeWidget(const QFont &initialFont, QWidget *parent = nullptr);
    int getSelected() const;

Q_SIGNALS:
    void selectedChanged();

private:
    void setListSelected(int size);

    IntItem *customItem;  // displays the value from `edit`
    QListWidget *list;
    QSpinBox *edit;
};

FontSizeWidget::FontSizeWidget(const QFont &initialFont, QWidget *parent)
    : QWidget(parent)
    , customItem(new IntItem)
    , list(new QListWidget)
    , edit(new QSpinBox)
{
    auto *layout = new QVBoxLayout;
    auto *header = new QHBoxLayout;

    this->setLayout(layout);

    this->list->setSortingEnabled(true);
    this->list->addItem(this->customItem);

    for (int size : QFontDatabase::standardSizes())
    {
        this->list->addItem(new IntItem(size));
    }

    this->edit->setValue(initialFont.pointSize());
    this->edit->setMinimum(1);

    this->setListSelected(initialFont.pointSize());

    layout->addLayout(header);
    layout->addWidget(this->list);
    layout->setContentsMargins(0, 0, 0, 0);

    header->addWidget(new QLabel("Size"));
    header->addWidget(this->edit);
    header->setContentsMargins(0, 0, 0, 0);

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

class FontFamiliesWidget : public QWidget
{
    Q_OBJECT

public:
    FontFamiliesWidget(const QFont &initialFont, QWidget *parent = nullptr);
    QString getSelected() const;

Q_SIGNALS:
    void selectedChanged();

private:
    void setSelected(const QString &family);

    QListView *list;
    QStringListModel *model;
    QSortFilterProxyModel *proxy;
};

QStringList getFontFamilies()
{
    QStringList families = QFontDatabase::families();
    families.removeIf(QFontDatabase::isPrivateFamily);
    return families;
}

FontFamiliesWidget::FontFamiliesWidget(const QFont &initialFont,
                                       QWidget *parent)
    : QWidget(parent)
    , list(new QListView)
    , model(new QStringListModel(getFontFamilies(), this))
    , proxy(new QSortFilterProxyModel(this))
{
    auto *layout = new QVBoxLayout;
    auto *header = new QHBoxLayout;
    auto *search = new QLineEdit;

    this->setLayout(layout);

    this->list->setModel(this->proxy);
    this->list->setEditTriggers(QAbstractItemView::NoEditTriggers);

    this->proxy->setSourceModel(this->model);
    this->proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    this->setSelected(initialFont.family());

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
        this, [this](const QModelIndex &proxyIndex, const QModelIndex &) {
            if (proxyIndex.isValid())
            {
                Q_EMIT this->selectedChanged();
            }
        });
}

void FontFamiliesWidget::setSelected(const QString &family)
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

QString FontFamiliesWidget::getSelected() const
{
    QModelIndex proxyIndex = this->list->currentIndex();

    if (!proxyIndex.isValid())
    {
        return {};
    }

    QModelIndex modelIndex = this->proxy->mapToSource(proxyIndex);
    return this->model->data(modelIndex).toString();
}

class FontWeightWidget : public QWidget
{
    Q_OBJECT

public:
    FontWeightWidget(const QFont &initialFont, QWidget *parent = nullptr);
    void setFamily(const QString &family);
    int getSelected() const;

Q_SIGNALS:
    void selectedChanged();

private:
    QListWidget *list;
};

FontWeightWidget::FontWeightWidget(const QFont &initialFont, QWidget *parent)
    : QWidget(parent)
    , list(new QListWidget)
{
    auto *layout = new QVBoxLayout;

    this->setLayout(layout);
    this->list->setSortingEnabled(true);

    this->setFamily(initialFont.family());

    if (IntItem *item = findIntItemInList(this->list, initialFont.weight()))
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
    QSet<int> weights;
    int defaultWeight = QFont(family).weight();
    auto *defaultItem = new IntItem(defaultWeight);

    QSignalBlocker listSignalBlocker(this->list);

    this->list->clear();
    this->list->addItem(defaultItem);

    weights.insert(defaultWeight);

    // the goal is to only display valid weights and this gets close, but
    // is not perfect for all fonts.
    for (const auto &style : QFontDatabase::styles(family))
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

int FontWeightWidget::getSelected() const
{
    auto *cast = dynamic_cast<IntItem *>(this->list->currentItem());
    return cast ? cast->getValue() : -1;
}

class FontDialog : public QDialog
{
    Q_OBJECT

public:
    FontDialog(const QFont &initialFont, QWidget *parent = nullptr);
    QFont getSelected() const;

private:
    void updateSampleFont();

    QPlainTextEdit *sampleBox;
    FontFamiliesWidget *fontFamiliesW;
    FontSizeWidget *fontSizeW;
    FontWeightWidget *fontWeightW;
};

FontDialog::FontDialog(const QFont &initialFont, QWidget *parent)
    : QDialog(parent)
    , sampleBox(new QPlainTextEdit)
    , fontFamiliesW(new FontFamiliesWidget(initialFont))
    , fontSizeW(new FontSizeWidget(initialFont))
    , fontWeightW(new FontWeightWidget(initialFont))
{
    auto *layout = new QVBoxLayout;
    auto *choiceLayout = new QHBoxLayout;
    auto *choiceSideLayout = new QVBoxLayout;
    auto *buttons = new QDialogButtonBox;

    this->setWindowTitle("Pick Font");
    this->setLayout(layout);
    this->resize(450, 450);

    this->sampleBox->setPlainText(
        "The quick brown fox jumps over the lazy dog");
    this->updateSampleFont();

    layout->addLayout(choiceLayout, 5);
    layout->addWidget(new QLabel("Sample"));
    layout->addWidget(this->sampleBox, 1);
    layout->addWidget(buttons);

    choiceLayout->addWidget(this->fontFamiliesW, 5);
    choiceLayout->addLayout(choiceSideLayout, 3);

    choiceSideLayout->addWidget(this->fontSizeW);
    choiceSideLayout->addWidget(this->fontWeightW);

    buttons->addButton("Ok", QDialogButtonBox::AcceptRole);
    buttons->addButton("Cancel", QDialogButtonBox::RejectRole);

    QObject::connect(buttons, &QDialogButtonBox::accepted, this,
                     &QDialog::accept);

    QObject::connect(buttons, &QDialogButtonBox::rejected, this,
                     &QDialog::reject);

    QObject::connect(
        this->fontFamiliesW, &FontFamiliesWidget::selectedChanged, this,
        [this] {
            this->fontWeightW->setFamily(this->fontFamiliesW->getSelected());
            this->updateSampleFont();
        });

    QObject::connect(this->fontWeightW, &FontWeightWidget::selectedChanged,
                     this, &FontDialog::updateSampleFont);

    QObject::connect(this->fontSizeW, &FontSizeWidget::selectedChanged, this,
                     &FontDialog::updateSampleFont);
}

QFont FontDialog::getSelected() const
{
    return {this->fontFamiliesW->getSelected(), this->fontSizeW->getSelected(),
            this->fontWeightW->getSelected()};
}

void FontDialog::updateSampleFont()
{
    QTextCharFormat format;
    QTextCursor cursor = this->sampleBox->textCursor();
    QFont font = this->getSelected();

    format.setFont(font);

    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(format);

    this->sampleBox->document()->setDefaultFont(font);
}

}  // namespace

void FontSettingWidget::updateCurrentLabel()
{
    QFont font = getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1);
    QString family = font.family();
    QString ptSize = QString::number(font.pointSize());
    this->currentLabel->setText(family + ", " + ptSize + "pt");
}

FontSettingWidget::FontSettingWidget(QWidget *parent)
    : QWidget(parent)
    , currentLabel(new QLabel)
{
    auto *layout = new QHBoxLayout;
    auto *button = new QPushButton;

    this->setLayout(layout);
    this->updateCurrentLabel();

    layout->addWidget(new QLabel("Font:"));
    layout->addStretch(1);
    layout->addWidget(this->currentLabel);
    layout->addWidget(button);
    layout->setContentsMargins(0, 0, 0, 0);

    button->setIcon(QIcon(":/buttons/edit.svg"));

    QObject::connect(button, &QPushButton::clicked, this, [this] {
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
        getApp()->getWindows()->forceLayoutChannelViews();
    });
}

}  // namespace chatterino

#include "FontSettingWidget.moc"
