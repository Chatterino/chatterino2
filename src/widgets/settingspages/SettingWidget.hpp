#pragma once

#include "common/ChatterinoSetting.hpp"
#include "util/QMagicEnum.hpp"
#include "widgets/settingspages/GeneralPageView.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QtContainerFwd>
#include <QWidget>

namespace chatterino {

class GeneralPageView;

class SettingWidget : QWidget
{
    Q_OBJECT

    explicit SettingWidget(const QString &mainKeyword);

public:
    ~SettingWidget() override = default;
    SettingWidget &operator=(const SettingWidget &) = delete;
    SettingWidget &operator=(SettingWidget &&) = delete;
    SettingWidget(const SettingWidget &other) = delete;
    SettingWidget(SettingWidget &&other) = delete;

    static SettingWidget *checkbox(const QString &label, BoolSetting &setting);
    static SettingWidget *inverseCheckbox(const QString &label,
                                          BoolSetting &setting);
    template <typename T>
    static SettingWidget *dropdown(const QString &label,
                                   EnumStringSetting<T> &setting)
    {
        auto *widget = new SettingWidget(label);

        auto *lbl = new QLabel(label % ":");
        auto *combo = new ComboBox;
        combo->setFocusPolicy(Qt::StrongFocus);
        for (const auto &item : qmagicenum::enumNames<T>())
        {
            combo->addItem(item.toString());
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
            [&setting](const auto &newText) {
                // The setter for EnumStringSetting does not check that this value is valid
                // Instead, it's up to the getters to make sure that the setting is legic - see the enum_cast above
                // You could also use the settings `getEnum` function
                setting = newText;
            });

        return widget;
    }

    SettingWidget *setTooltip(QString tooltip);
    SettingWidget *setDescription(const QString &text);

    /// Add extra keywords to the widget
    ///
    /// All text from the tooltip, description, and label are already keywords
    SettingWidget *addKeywords(const QStringList &newKeywords);

    void addTo(GeneralPageView &view);

private:
    QWidget *label = nullptr;
    QWidget *actionWidget = nullptr;

    QVBoxLayout *vLayout;
    QHBoxLayout *hLayout;

    pajlada::Signals::SignalHolder managedConnections;

    QStringList keywords;
};

}  // namespace chatterino
