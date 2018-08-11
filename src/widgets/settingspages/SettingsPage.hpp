#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <pajlada/signals/signal.hpp>

#include "singletons/Settings.hpp"

namespace chatterino {

class SettingsPage : public QWidget
{
public:
    SettingsPage(const QString &name, const QString &iconResource);

    const QString &getName();
    const QString &getIconResource();

    void cancel();

    QCheckBox *createCheckBox(const QString &text,
                              pajlada::Settings::Setting<bool> &setting);
    QComboBox *createComboBox(const QStringList &items,
                              pajlada::Settings::Setting<QString> &setting);
    QLineEdit *createLineEdit(pajlada::Settings::Setting<QString> &setting);
    QSpinBox *createSpinBox(pajlada::Settings::Setting<int> &setting,
                            int min = 0, int max = 2500);

    virtual void onShow()
    {
    }

protected:
    QString name_;
    QString iconResource_;

    pajlada::Signals::NoArgSignal onCancel_;
    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
};

}  // namespace chatterino
