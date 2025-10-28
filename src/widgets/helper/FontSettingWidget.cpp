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
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStringListModel>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QToolButton>
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

class PreviewWidget : public QWidget
{
public:
    void paintEvent(QPaintEvent * /* event */) override
    {
        QPainter painter{this};

        painter.fillRect(this->rect(), this->palette().base());
        painter.setFont(this->font);
        painter.drawText(
            this->rect().adjusted(3, 3, -3, -3),
            Qt::AlignCenter | Qt::TextSingleLine,
            QStringLiteral("The quick brown fox jumps over the lazy dog"));
    }

    void setFont(const QFont &newFont)
    {
        this->font = newFont;
        this->update();
    }

private:
    QFont font;
};

class FontDialog : public QDialog
{
    Q_OBJECT

public:
    FontDialog(const QFont &initialFont, QWidget *parent = nullptr);
    QFont getSelected() const;

Q_SIGNALS:
    void applyRequested();

private:
    void updatePreview();

    PreviewWidget *preview;
    FontFamiliesWidget *fontFamilies;
    FontSizeWidget *fontSize;
    FontWeightWidget *fontWeight;
};

FontDialog::FontDialog(const QFont &initialFont, QWidget *parent)
    : QDialog(parent)
    , preview(new PreviewWidget)
    , fontFamilies(new FontFamiliesWidget(initialFont))
    , fontSize(new FontSizeWidget(initialFont))
    , fontWeight(new FontWeightWidget(initialFont))
{
    auto *layout = new QVBoxLayout;
    auto *choiceLayout = new QHBoxLayout;
    auto *choiceSideLayout = new QVBoxLayout;
    auto *buttonLayout = new QHBoxLayout;

    auto *applyButton = new QPushButton("Apply");
    auto *acceptButton = new QPushButton("Accept");
    auto *rejectButton = new QPushButton("Cancel");

    this->setWindowTitle("Pick Font");
    this->setLayout(layout);
    this->resize(450, 450);

    layout->addLayout(choiceLayout, 5);
    layout->addWidget(new QLabel("Preview"));
    layout->addWidget(this->preview, 1);
    layout->addLayout(buttonLayout);

    choiceLayout->addWidget(this->fontFamilies, 5);
    choiceLayout->addLayout(choiceSideLayout, 3);

    choiceSideLayout->addWidget(this->fontSize);
    choiceSideLayout->addWidget(this->fontWeight);

    buttonLayout->addWidget(applyButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(rejectButton);

    this->preview->setMinimumHeight(60);
    this->updatePreview();

    QObject::connect(applyButton, &QPushButton::clicked, this,
                     &FontDialog::applyRequested);

    QObject::connect(acceptButton, &QPushButton::clicked, this,
                     &QDialog::accept);

    QObject::connect(rejectButton, &QPushButton::clicked, this,
                     &QDialog::reject);

    QObject::connect(
        this->fontFamilies, &FontFamiliesWidget::selectedChanged, this, [this] {
            this->fontWeight->setFamily(this->fontFamilies->getSelected());
            this->updatePreview();
        });

    QObject::connect(this->fontWeight, &FontWeightWidget::selectedChanged, this,
                     &FontDialog::updatePreview);

    QObject::connect(this->fontSize, &FontSizeWidget::selectedChanged, this,
                     &FontDialog::updatePreview);
}

QFont FontDialog::getSelected() const
{
    return {this->fontFamilies->getSelected(), this->fontSize->getSelected(),
            this->fontWeight->getSelected()};
}

void FontDialog::updatePreview()
{
    this->preview->setFont(this->getSelected());
}

class FontSettingDialog : public FontDialog
{
    Q_OBJECT

public:
    FontSettingDialog(QWidget *parent = nullptr)
        : FontDialog(getApp()->getFonts()->getFont(FontStyle::ChatMedium, 1),
                     parent)
        , oldFont(getSettings()->chatFontFamily)
        , oldSize(getSettings()->chatFontSize)
        , oldWeight(getSettings()->chatFontWeight)
    {
        QObject::connect(this, &FontDialog::applyRequested, [this] {
            this->needRestore = true;
            this->setSettings();
        });
        QObject::connect(this, &FontDialog::rejected, [this] {
            if (this->needRestore)
            {
                getSettings()->chatFontFamily = this->oldFont;
                getSettings()->chatFontSize = this->oldSize;
                getSettings()->chatFontWeight = this->oldWeight;
            }
        });
        QObject::connect(this, &FontDialog::accepted, [this] {
            this->setSettings();
        });
    }

private:
    void setSettings()
    {
        QFont selected = this->getSelected();
        getSettings()->chatFontFamily = selected.family();
        getSettings()->chatFontSize = selected.pointSize();
        getSettings()->chatFontWeight = selected.weight();
    }

    QString oldFont;
    int oldSize;
    int oldWeight;
    bool needRestore{};
};

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
    , listener([this] {
        this->updateCurrentLabel();
    })
{
    auto *layout = new QHBoxLayout;
    auto *button = new QToolButton;

    this->setLayout(layout);
    this->updateCurrentLabel();

    this->listener.add(getApp()->getFonts()->fontChanged);

    layout->addWidget(new QLabel("Font:"));
    layout->addStretch(1);
    layout->addWidget(this->currentLabel);
    layout->addWidget(button);
    layout->setContentsMargins(0, 0, 0, 0);

    button->setIcon(QIcon(":/buttons/edit.svg"));

    QObject::connect(button, &QPushButton::clicked, this, [this] {
        FontSettingDialog{this}.exec();
    });
}

}  // namespace chatterino

#include "FontSettingWidget.moc"
