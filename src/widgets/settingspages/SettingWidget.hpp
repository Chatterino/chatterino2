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
                                   EnumStringSetting<T> &setting);

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
