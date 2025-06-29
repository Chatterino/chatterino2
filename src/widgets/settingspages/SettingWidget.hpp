#pragma once

#include "common/ChatterinoSetting.hpp"

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

#include <functional>
#include <optional>

class QFormLayout;

namespace chatterino {

class GeneralPageView;

class SettingWidget : public QWidget
{
    Q_OBJECT

    explicit SettingWidget(const QString &mainKeyword);

public:
    struct IntInputParams {
        /// The minimum value of this spin box.
        /// Leave empty for a minimum value of 0.
        std::optional<int> min;

        /// The maximum value of this spin box.
        /// Leave empty for a maximum value of 99.
        std::optional<int> max;

        /// The value the spinbox is incremented or decremented by when the up or down arrow is clicked.
        /// Leave empty for a single step of 1.
        std::optional<int> singleStep;

        /// The suffix appended to the end of the displayed value.
        /// Leave empty for no suffix.
        std::optional<QString> suffix;
    };

    ~SettingWidget() override = default;
    SettingWidget &operator=(const SettingWidget &) = delete;
    SettingWidget &operator=(SettingWidget &&) = delete;
    SettingWidget(const SettingWidget &other) = delete;
    SettingWidget(SettingWidget &&other) = delete;

    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        checkbox(const QString &label, BoolSetting &setting);
    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        inverseCheckbox(const QString &label, BoolSetting &setting);
    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        customCheckbox(const QString &label, bool initialValue,
                       const std::function<void(bool)> &save);

    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        intInput(const QString &label, IntSetting &setting,
                 IntInputParams params);

    template <typename T>
    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        dropdown(const QString &label, EnumStringSetting<T> &setting);

    template <typename T>
    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        dropdown(const QString &label, EnumSetting<T> &setting);

    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        colorButton(const QString &label, QStringSetting &setting);
    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        lineEdit(const QString &label, QStringSetting &setting,
                 const QString &placeholderText = {});

    [[nodiscard("Must use created setting widget")]] static SettingWidget *
        fontButton(const QString &label, QStringSetting &familySetting,
                   std::function<QFont()> currentFont,
                   std::function<void(QFont)> onChange);

    [[nodiscard("Must use created setting widget")]] SettingWidget *setTooltip(
        QString tooltip);
    [[nodiscard("Must use created setting widget")]] SettingWidget *
        setDescription(const QString &text);

    /// Add extra keywords to the widget
    ///
    /// All text from the tooltip, description, and label are already keywords
    [[nodiscard("Must use created setting widget")]] SettingWidget *addKeywords(
        const QStringList &newKeywords);

    [[nodiscard("Must use created setting widget")]] SettingWidget *
        conditionallyEnabledBy(BoolSetting &setting);

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
