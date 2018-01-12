#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <pajlada/signals/signal.hpp>

#include "singletons/settingsmanager.hpp"

namespace chatterino {
namespace widgets {
namespace settingspages {

class SettingsPage : public QWidget
{
public:
    SettingsPage(const QString &name, const QString &iconResource);

    const QString &getName();
    const QString &getIconResource();

    void cancel();

    QCheckBox *createCheckBox(const QString &text, pajlada::Settings::Setting<bool> &setting);
    QComboBox *createComboBox(const QStringList &items,
                              pajlada::Settings::Setting<QString> &setting);
    QLineEdit *createLineEdit(pajlada::Settings::Setting<QString> &setting);

protected:
    QString name;
    QString iconResource;

    pajlada::Signals::NoArgSignal onCancel;
    std::vector<pajlada::Signals::ScopedConnection> managedConnections;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
