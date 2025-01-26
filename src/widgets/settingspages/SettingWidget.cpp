#include "widgets/settingspages/SettingWidget.hpp"

#include "widgets/settingspages/GeneralPageView.hpp"

#include <QBoxLayout>
#include <QCheckBox>
#include <QLabel>

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

void SettingWidget::addTo(GeneralPageView &view)
{
    view.addWidget(this, this->keywords);
}

}  // namespace chatterino
