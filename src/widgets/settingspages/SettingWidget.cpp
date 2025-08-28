#include "widgets/settingspages/SettingWidget.hpp"

#include "common/QLogging.hpp"
#include "singletons/Settings.hpp"  // IWYU pragma: keep
#include "util/QMagicEnumTagged.hpp"
#include "util/RapidJsonSerializeQString.hpp"  // IWYU pragma: keep
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/color/ColorButton.hpp"
#include "widgets/settingspages/CustomWidgets.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <QFontDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>

namespace {

constexpr int MAX_TOOLTIP_LINE_LENGTH = 50;
const auto MAX_TOOLTIP_LINE_LENGTH_PATTERN =
    QStringLiteral(R"(.{%1}\S*\K(\s+))").arg(MAX_TOOLTIP_LINE_LENGTH);
const QRegularExpression MAX_TOOLTIP_LINE_LENGTH_REGEX(
    MAX_TOOLTIP_LINE_LENGTH_PATTERN);

}  // namespace

namespace chatterino {

SettingWidget::SettingWidget(const QString &mainKeyword)
    : vLayout(new QVBoxLayout(this))
    , hLayout(new QHBoxLayout)
{
    this->vLayout->setContentsMargins(0, 0, 0, 0);

    this->hLayout->setContentsMargins(0, 0, 0, 0);
    this->vLayout->addLayout(hLayout);

    this->keywords.append(mainKeyword);
}

SettingWidget *SettingWidget::checkbox(const QString &label,
                                       BoolSetting &setting)
{
    auto *widget = new SettingWidget(label);

    auto *check = new SCheckBox(label);

    widget->hLayout->addWidget(check);

    // update when setting changes
    setting.connect(
        [check](const bool &value) {
            check->setChecked(value);
        },
        widget->managedConnections);

    // update setting on toggle
    QObject::connect(check, &QCheckBox::toggled, widget,
                     [&setting](bool state) {
                         setting = state;
                     });

    widget->actionWidget = check;
    widget->label = check;

    return widget;
}

SettingWidget *SettingWidget::inverseCheckbox(const QString &label,
                                              BoolSetting &setting)
{
    auto *widget = new SettingWidget(label);

    auto *check = new SCheckBox(label);

    widget->hLayout->addWidget(check);

    // update when setting changes
    setting.connect(
        [check](const bool &value) {
            check->setChecked(!value);
        },
        widget->managedConnections);

    // update setting on toggle
    QObject::connect(check, &QCheckBox::toggled, widget,
                     [&setting](bool state) {
                         setting = !state;
                     });

    widget->actionWidget = check;
    widget->label = check;

    return widget;
}

SettingWidget *SettingWidget::customCheckbox(
    const QString &label, bool initialValue,
    const std::function<void(bool)> &save)
{
    auto *widget = new SettingWidget(label);

    auto *check = new SCheckBox(label);

    widget->hLayout->addWidget(check);

    check->setChecked(initialValue);

    QObject::connect(check, &QCheckBox::toggled, widget, save);

    widget->actionWidget = check;
    widget->label = check;

    return widget;
}

SettingWidget *SettingWidget::intInput(const QString &label,
                                       IntSetting &setting,
                                       IntInputParams params)
{
    auto *widget = new SettingWidget(label);

    auto *lbl = new QLabel(label + ":");

    auto *input = new SpinBox;
    if (params.min.has_value())
    {
        input->setMinimum(params.min.value());
    }
    if (params.max.has_value())
    {
        input->setMaximum(params.max.value());
    }
    if (params.singleStep.has_value())
    {
        input->setSingleStep(params.singleStep.value());
    }
    if (params.suffix.has_value())
    {
        input->setSuffix(params.suffix.value());
    }

    widget->hLayout->addWidget(lbl);
    widget->hLayout->addStretch(1);
    widget->hLayout->addWidget(input);

    // update when setting changes
    setting.connect(
        [input](const int &value, const auto &) {
            input->setValue(value);
        },
        widget->managedConnections);

    // update setting on value changed
    QObject::connect(input, QOverload<int>::of(&QSpinBox::valueChanged), widget,
                     [&setting](int newValue) {
                         setting = newValue;
                     });

    widget->actionWidget = input;
    widget->label = lbl;

    return widget;
}

template <typename T>
SettingWidget *SettingWidget::dropdown(const QString &label,
                                       EnumStringSetting<T> &setting)
{
    auto *widget = new SettingWidget(label);

    auto *lbl = new QLabel(label % ":");
    auto *combo = new ComboBox;
    combo->setFocusPolicy(Qt::StrongFocus);

    for (const auto value : magic_enum::enum_values<T>())
    {
        combo->addItem(qmagicenum::enumDisplayNameString(value),
                       QVariant(static_cast<std::underlying_type_t<T>>(value)));
    }

    // TODO: this can probably use some other size hint/size strategy
    combo->setMinimumWidth(combo->minimumSizeHint().width() + 30);

    widget->actionWidget = combo;
    widget->label = lbl;

    widget->hLayout->addWidget(lbl);
    widget->hLayout->addStretch(1);
    widget->hLayout->addWidget(combo);

    setting.connect(
        [&setting, combo](const QString &value) {
            auto enumValue =
                qmagicenum::enumCast<T>(value, qmagicenum::CASE_INSENSITIVE)
                    .value_or(setting.defaultValue);

            auto i = magic_enum::enum_index(enumValue).value_or(0);

            combo->setCurrentIndex(i);
        },
        widget->managedConnections);

    QObject::connect(
        combo, &QComboBox::currentTextChanged,
        [label, combo, &setting](const auto &newText) {
            bool ok = true;
            auto enumValue = combo->currentData().toInt(&ok);
            if (!ok)
            {
                qCWarning(chatterinoWidget)
                    << "Combo" << label << " with value" << newText
                    << "did not contain an intable UserRole data";
                return;
            }

            setting = qmagicenum::enumNameString(static_cast<T>(enumValue));
        });

    return widget;
}

template SettingWidget *SettingWidget::dropdown<SoundBackend>(
    const QString &label, EnumStringSetting<SoundBackend> &setting);
template SettingWidget *SettingWidget::dropdown<EmoteTooltipScale>(
    const QString &label, EnumStringSetting<EmoteTooltipScale> &setting);
template SettingWidget *SettingWidget::dropdown<StreamLinkPreferredQuality>(
    const QString &label,
    EnumStringSetting<StreamLinkPreferredQuality> &setting);
template SettingWidget *SettingWidget::dropdown<ChatSendProtocol>(
    const QString &label, EnumStringSetting<ChatSendProtocol> &setting);
template SettingWidget *SettingWidget::dropdown<TabStyle>(
    const QString &label, EnumStringSetting<TabStyle> &setting);
template SettingWidget *SettingWidget::dropdown<ShowModerationState>(
    const QString &label, EnumStringSetting<ShowModerationState> &setting);
template SettingWidget *SettingWidget::dropdown<EmojiStyle>(
    const QString &label, EnumStringSetting<EmojiStyle> &setting);

template <typename T>
SettingWidget *SettingWidget::dropdown(const QString &label,
                                       EnumSetting<T> &setting)
{
    auto *widget = new SettingWidget(label);

    auto *lbl = new QLabel(label % ":");
    auto *combo = new ComboBox;
    combo->setFocusPolicy(Qt::StrongFocus);

    for (const auto value : magic_enum::enum_values<T>())
    {
        combo->addItem(qmagicenum::enumDisplayNameString(value),
                       QVariant(static_cast<std::underlying_type_t<T>>(value)));
    }

    // TODO: this can probably use some other size hint/size strategy
    combo->setMinimumWidth(combo->minimumSizeHint().width() + 30);

    widget->actionWidget = combo;
    widget->label = lbl;

    widget->hLayout->addWidget(lbl);
    widget->hLayout->addStretch(1);
    widget->hLayout->addWidget(combo);

    setting.connect(
        [combo, label](const auto &value) {
            std::optional<int> foundRow;

            for (auto row = 0; row < combo->model()->rowCount(); ++row)
            {
                auto index = combo->model()->index(row, 0);
                auto rowEnumValue = index.data(Qt::UserRole);
                if (rowEnumValue == value)
                {
                    foundRow = row;
                    break;
                }
            }

            if (foundRow)
            {
                combo->setCurrentIndex(*foundRow);
            }
            else
            {
                qCWarning(chatterinoWidget)
                    << "Did not find a correct combo box row for" << label
                    << " with value" << value;
            }
        },
        widget->managedConnections);

    QObject::connect(combo, &QComboBox::currentTextChanged,
                     [label, combo, &setting](const auto &newText) {
                         bool ok = true;
                         auto enumValue = combo->currentData().toInt(&ok);
                         if (!ok)
                         {
                             qCWarning(chatterinoWidget)
                                 << "Combo" << label << " with value" << newText
                                 << "did not contain an intable UserRole data";
                             return;
                         }

                         setting.setValue(enumValue);
                     });

    return widget;
}

template SettingWidget *SettingWidget::dropdown<LastMessageLineStyle>(
    const QString &label, EnumSetting<LastMessageLineStyle> &setting);
template SettingWidget *SettingWidget::dropdown<ThumbnailPreviewMode>(
    const QString &label, EnumSetting<ThumbnailPreviewMode> &setting);
template SettingWidget *SettingWidget::dropdown<StreamerModeSetting>(
    const QString &label, EnumSetting<StreamerModeSetting> &setting);

SettingWidget *SettingWidget::dropdown(
    const QString &label, QStringSetting &setting,
    const std::vector<std::pair<QString, QVariant>> &items)
{
    auto *widget = new SettingWidget(label);

    auto *lbl = new QLabel(label % ":");
    auto *combo = new ComboBox;
    combo->setFocusPolicy(Qt::StrongFocus);

    for (const auto &[itemText, itemData] : items)
    {
        combo->addItem(itemText, itemData);
    }

    // TODO: this can probably use some other size hint/size strategy
    combo->setMinimumWidth(combo->minimumSizeHint().width() + 30);

    widget->actionWidget = combo;
    widget->label = lbl;

    widget->hLayout->addWidget(lbl);
    widget->hLayout->addStretch(1);
    widget->hLayout->addWidget(combo);

    setting.connect(
        [combo, label](const auto &value) {
            std::optional<int> foundRow;

            for (auto row = 0; row < combo->model()->rowCount(); ++row)
            {
                auto index = combo->model()->index(row, 0);
                auto rowEnumValue = index.data(Qt::UserRole);
                if (rowEnumValue == value)
                {
                    foundRow = row;
                    break;
                }
            }

            if (foundRow)
            {
                combo->setCurrentIndex(*foundRow);
            }
            else
            {
                qCWarning(chatterinoWidget)
                    << "Did not find a correct combo box row for" << label
                    << " with value" << value;
            }
        },
        widget->managedConnections);

    QObject::connect(combo, &QComboBox::currentTextChanged,
                     [label, combo, &setting](const auto &newText) {
                         bool ok = true;
                         auto stringValue = combo->currentData().toString();

                         setting.setValue(stringValue);
                     });

    return widget;
}

SettingWidget *SettingWidget::colorButton(const QString &label,
                                          QStringSetting &setting)
{
    QColor color(setting.getValue());
    auto *widget = new SettingWidget(label);

    auto *lbl = new QLabel(label + ":");

    auto *colorButton = new ColorButton(color);

    widget->hLayout->addWidget(lbl);
    widget->hLayout->addStretch(1);
    widget->hLayout->addWidget(colorButton);

    // update when setting changes
    setting.connect(
        [colorButton](const QString &value, const auto &) {
            colorButton->setColor(QColor(value));
        },
        widget->managedConnections);

    QObject::connect(colorButton, &ColorButton::clicked, [widget, &setting]() {
        auto *dialog = new ColorPickerDialog(QColor(setting), widget);
        // colorButton & setting are never deleted and the signal is deleted
        // once the dialog is closed
        QObject::connect(
            dialog, &ColorPickerDialog::colorConfirmed, widget,
            [&setting](auto selected) {
                if (selected.isValid())
                {
                    setting.setValue(selected.name(QColor::HexArgb));
                }
            });
        dialog->show();
    });

    widget->actionWidget = colorButton;
    widget->label = lbl;

    return widget;
}

SettingWidget *SettingWidget::lineEdit(const QString &label,
                                       QStringSetting &setting,
                                       const QString &placeholderText)
{
    auto *widget = new SettingWidget(label);

    auto *lbl = new QLabel(label + ":");

    auto *edit = new QLineEdit;
    edit->setText(setting);
    if (!placeholderText.isEmpty())
    {
        edit->setPlaceholderText(placeholderText);
    }

    widget->hLayout->addWidget(lbl);
    widget->hLayout->addWidget(edit);

    // Update the setting when the widget changes.
    QObject::connect(edit, &QLineEdit::textChanged,
                     [&setting](const QString &newValue) {
                         setting = newValue;
                     });

    // Update the widget to reflect the new setting value if the setting changes
    // This _will_ fire every time the widget changes, so we are being conservative
    // with the `setText` call to ensure the user doesn't get their cursor bounced around.
    setting.connect(
        [edit](const QString &value) {
            if (edit->text() != value)
            {
                edit->setText(value);
            }
        },
        widget->managedConnections, false);

    widget->actionWidget = edit;
    widget->label = lbl;

    return widget;
}

SettingWidget *SettingWidget::fontButton(const QString &label,
                                         QStringSetting &familySetting,
                                         std::function<QFont()> currentFont,
                                         std::function<void(QFont)> onChange)
{
    auto *widget = new SettingWidget(label);

    auto *lbl = new QLabel(label + ":");

    auto *button = new SPushButton(currentFont().family());

    widget->hLayout->addWidget(lbl);
    widget->hLayout->addStretch(1);
    widget->hLayout->addWidget(button);

    familySetting.connect(
        [button, currentFont](const auto &) {
            button->setText(currentFont().family());
        },
        widget->managedConnections);

    QObject::connect(button, &QPushButton::clicked,
                     [widget, currentFont{std::move(currentFont)},
                      onChange{std::move(onChange)}]() {
                         bool ok = false;
                         auto font =
                             QFontDialog::getFont(&ok, currentFont(), widget);
                         if (ok)
                         {
                             onChange(font);
                         }
                     });

    widget->actionWidget = button;
    widget->label = lbl;

    return widget;
}

SettingWidget *SettingWidget::setTooltip(QString tooltip)
{
    assert(!tooltip.isEmpty());

    if (tooltip.length() > MAX_TOOLTIP_LINE_LENGTH)
    {
        // match MAX_TOOLTIP_LINE_LENGTH characters, any remaining
        // non-space, and then capture the following space for
        // replacement with newline
        tooltip.replace(MAX_TOOLTIP_LINE_LENGTH_REGEX, "\n");
    }

    if (this->label != nullptr)
    {
        this->label->setToolTip(tooltip);
    }

    if (this->actionWidget != nullptr)
    {
        this->actionWidget->setToolTip(tooltip);
    }

    this->keywords.append(tooltip);

    return this;
}

SettingWidget *SettingWidget::setDescription(const QString &text)
{
    auto *lbl = new QLabel(text);
    lbl->setTextInteractionFlags(Qt::TextBrowserInteraction |
                                 Qt::LinksAccessibleByKeyboard);
    lbl->setOpenExternalLinks(true);
    lbl->setWordWrap(true);
    lbl->setObjectName("description");

    this->vLayout->insertWidget(0, lbl);

    this->keywords.append(text);

    return this;
}

SettingWidget *SettingWidget::addKeywords(const QStringList &newKeywords)
{
    this->keywords.append(newKeywords);

    return this;
}

SettingWidget *SettingWidget::conditionallyEnabledBy(BoolSetting &setting)
{
    setting.connect(
        [this](const auto &value, const auto &) {
            this->actionWidget->setEnabled(value);
        },
        this->managedConnections);

    return this;
}

SettingWidget *SettingWidget::conditionallyEnabledBy(
    QStringSetting &setting, const QString &expectedValue)
{
    setting.connect(
        [this, expectedValue](const auto &value, const auto &) {
            this->actionWidget->setEnabled(value == expectedValue);
        },
        this->managedConnections);

    return this;
}

void SettingWidget::addTo(GeneralPageView &view)
{
    view.pushWidget(this);

    this->registerWidget(view);
}

void SettingWidget::addTo(GeneralPageView &view, QFormLayout *formLayout)
{
    this->registerWidget(view);

    formLayout->addRow(this->label, this->actionWidget);
}

void SettingWidget::addToLayout(QLayout *layout)
{
    if (this->label == this->actionWidget)
    {
        layout->addWidget(this->actionWidget);
        return;
    }

    assert(false && "unimplemented");
}

void SettingWidget::registerWidget(GeneralPageView &view)
{
    if (this->label != nullptr)
    {
        view.registerWidget(this->label, this->keywords, this);
    }
    view.registerWidget(this->actionWidget, this->keywords, this);
}

}  // namespace chatterino
