#pragma once

#include "widgets/BasePopup.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace chatterino {

class QualityPopup : public BasePopup
{
public:
    QualityPopup(const QString &_channelName, QStringList options);
    static void showDialog(const QString &_channelName, QStringList options);

private:
    void okButtonClicked();
    void cancelButtonClicked();

    struct {
        QVBoxLayout vbox;
        QComboBox selector;
        QDialogButtonBox buttonBox;
        QPushButton okButton;
        QPushButton cancelButton;
    } ui_;

    QString channelName_;
};

}  // namespace chatterino
