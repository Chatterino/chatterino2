#pragma once

#include "ab/BaseWindow.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino
{
    class QualityPopup : public ab::BaseWindow
    {
    public:
        QualityPopup(const QString& _channelName, QStringList options);
        static void showDialog(
            const QString& _channelName, QStringList options);

    private:
        void okButtonClicked();
        void cancelButtonClicked();

        struct
        {
            QVBoxLayout vbox;
            QComboBox selector;
            QDialogButtonBox buttonBox;
            QPushButton okButton;
            QPushButton cancelButton;
        } ui_;

        QString channelName_;
    };
}  // namespace chatterino
