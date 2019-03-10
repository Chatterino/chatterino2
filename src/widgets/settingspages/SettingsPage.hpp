#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <pajlada/signals/signal.hpp>

#include "singletons/Settings.hpp"

namespace chatterino
{
    class SettingsDialogTab;

    class SettingsPage : public QFrame
    {
        Q_OBJECT

    public:
        SettingsPage(const QString& name, const QString& iconResource);

        const QString& getName();
        const QString& getIconResource();

        SettingsDialogTab* tab() const;
        void setTab(SettingsDialogTab* tab);

        void cancel();

        QCheckBox* createCheckBox(
            const QString& text, pajlada::Settings::Setting<bool>& setting);
        QComboBox* createComboBox(const QStringList& items,
            pajlada::Settings::Setting<QString>& setting);
        QLineEdit* createLineEdit(pajlada::Settings::Setting<QString>& setting);
        QSpinBox* createSpinBox(pajlada::Settings::Setting<int>& setting,
            int min = 0, int max = 2500);

        virtual void onShow()
        {
        }

    protected:
        QString name_;
        QString iconResource_;

        SettingsDialogTab* tab_;

        pajlada::Signals::NoArgSignal onCancel_;
        std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
    };

}  // namespace chatterino
