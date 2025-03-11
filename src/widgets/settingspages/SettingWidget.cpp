#include "widgets/settingspages/SettingWidget.hpp"

#include "util/RapidJsonSerializeQString.hpp"  // IWYU pragma: keep
#include "widgets/dialogs/ColorPickerDialog.hpp"
#include "widgets/helper/color/ColorButton.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"

#include <QBoxLayout>
#include <QCheckBox>
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

    auto *check = new QCheckBox(label);

    widget->hLayout->addWidget(check);

    // update when setting changes
    setting.connect(
        [check](const bool &value, auto) {
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

    auto *check = new QCheckBox(label);

    widget->hLayout->addWidget(check);

    // update when setting changes
    setting.connect(
        [check](const bool &value, auto) {
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

    auto *check = new QCheckBox(label);

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

    auto *input = new QSpinBox;
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
    QColor color(setting.getValue());
    auto *widget = new SettingWidget(label);

    auto *lbl = new QLabel(label + ":");

    auto *edit = new QLineEdit;
    edit->setText(setting);
    if (!placeholderText.isEmpty())
    {
        edit->setPlaceholderText(placeholderText);
    }

    widget->hLayout->addWidget(lbl);
    // widget->hLayout->addStretch(1);
    widget->hLayout->addWidget(edit);

    // update when setting changes
    QObject::connect(edit, &QLineEdit::textChanged,
                     [&setting](const QString &newValue) {
                         setting = newValue;
                     });

    widget->actionWidget = edit;
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

void SettingWidget::addTo(GeneralPageView &view)
{
    view.pushWidget(this);

    if (this->label != nullptr)
    {
        view.registerWidget(this->label, this->keywords, this);
    }
    view.registerWidget(this->actionWidget, this->keywords, this);
}

void SettingWidget::addTo(GeneralPageView &view, QFormLayout *formLayout)
{
    if (this->label != nullptr)
    {
        view.registerWidget(this->label, this->keywords, this);
    }
    view.registerWidget(this->actionWidget, this->keywords, this);

    formLayout->addRow(this->label, this->actionWidget);
}

}  // namespace chatterino
