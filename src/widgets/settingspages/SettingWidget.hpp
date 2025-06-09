#pragma once

#include "common/ChatterinoSetting.hpp"
#include "common/QLogging.hpp"
#include "util/QMagicEnum.hpp"
#include "util/QMagicEnumTagged.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QBoxLayout>
#include <QComboBox>
#include <QDebug>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QStringBuilder>
#include <QStringList>
#include <QtContainerFwd>
#include <QWidget>

class QFormLayout;

namespace chatterino {

class GeneralPageView;

class SettingWidget : QWidget
{
    Q_OBJECT

    explicit SettingWidget(const QString &mainKeyword);

public:
    struct IntInputParams {
        std::optional<int> min;
        std::optional<int> max;
        std::optional<int> singleStep;
    };

    ~SettingWidget() override = default;
    SettingWidget &operator=(const SettingWidget &) = delete;
    SettingWidget &operator=(SettingWidget &&) = delete;
    SettingWidget(const SettingWidget &other) = delete;
    SettingWidget(SettingWidget &&other) = delete;

    static SettingWidget *checkbox(const QString &label, BoolSetting &setting);
    static SettingWidget *inverseCheckbox(const QString &label,
                                          BoolSetting &setting);
    static SettingWidget *customCheckbox(const QString &label,
                                         bool initialValue,
                                         const std::function<void(bool)> &save);

    static SettingWidget *intInput(const QString &label, IntSetting &setting,
                                   IntInputParams params);

    template <typename T>
    static SettingWidget *dropdown(const QString &label,
                                   EnumStringSetting<T> &setting)
    {
        auto *widget = new SettingWidget(label);

        auto *lbl = new QLabel(label % ":");
        auto *combo = new ComboBox;
        combo->setFocusPolicy(Qt::StrongFocus);

        for (const auto value : magic_enum::enum_values<T>())
        {
            combo->addItem(
                qmagicenum::enumDisplayNameString(value),
                QVariant(static_cast<std::underlying_type_t<T>>(value)));
        }

        // TODO: this can probably use some other size hint/size strategy
        combo->setMinimumWidth(combo->minimumSizeHint().width());

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

                auto i = magic_enum::enum_integer(enumValue);

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
    static SettingWidget *colorButton(const QString &label,
                                      QStringSetting &setting);
    static SettingWidget *lineEdit(const QString &label,
                                   QStringSetting &setting,
                                   const QString &placeholderText = {});

    SettingWidget *setTooltip(QString tooltip);
    SettingWidget *setDescription(const QString &text);

    /// Add extra keywords to the widget
    ///
    /// All text from the tooltip, description, and label are already keywords
    SettingWidget *addKeywords(const QStringList &newKeywords);

    SettingWidget *conditionallyEnabledBy(BoolSetting &setting);

    void addTo(GeneralPageView &view);
    void addTo(GeneralPageView &view, QFormLayout *formLayout);

private:
    QWidget *label = nullptr;
    QWidget *actionWidget = nullptr;

    QVBoxLayout *vLayout;
    QHBoxLayout *hLayout;

    pajlada::Signals::SignalHolder managedConnections;

    QStringList keywords;
};

}  // namespace chatterino
