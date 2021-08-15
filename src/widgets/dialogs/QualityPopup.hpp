#pragma once

#include "widgets/BasePopup.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace chatterino {

class QualityPopup : public BasePopup
{
public:
    QualityPopup(const QString &_channelName, QStringList options);
    static void showDialog(const QString &_channelName, QStringList options);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    void okButtonClicked();
    void cancelButtonClicked();

    struct {
        QVBoxLayout *vbox;
        QComboBox *selector;
        QDialogButtonBox *buttonBox;
    } ui_;

    QString channelName_;
};

}  // namespace chatterino
