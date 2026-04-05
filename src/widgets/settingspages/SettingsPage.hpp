// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/settingspages/CustomWidgets.hpp"

#include <pajlada/settings.hpp>
#include <pajlada/signals/signal.hpp>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>

namespace chatterino {

class SettingsDialogTab;

class SettingsPage : public QFrame
{
    Q_OBJECT

public:
    SettingsPage();

    virtual bool filterElements(const QString &query);

    SettingsDialogTab *tab() const;
    void setTab(SettingsDialogTab *tab);

    QCheckBox *createCheckBox(const QString &text,
                              pajlada::Settings::Setting<bool> &setting,
                              const QString &toolTipText = {});
    QComboBox *createComboBox(const QStringList &items,
                              pajlada::Settings::Setting<QString> &setting);
    QSpinBox *createSpinBox(pajlada::Settings::Setting<int> &setting,
                            int min = 0, int max = 2500);
    template <typename T>
    SLabel *createLabel(const std::function<QString(const T &)> &makeText,
                        pajlada::Settings::Setting<T> &setting)
    {
        auto *label = new SLabel();

        setting.connect(
            [label, makeText](const T &value, auto) {
                label->setText(makeText(value));
            },
            this->managedConnections_);

        return label;
    }

    virtual void onShow()
    {
    }

protected:
    SettingsDialogTab *tab_{};
    pajlada::Signals::SignalHolder managedConnections_;
};

}  // namespace chatterino
