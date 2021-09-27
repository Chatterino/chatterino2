#pragma once

#include "widgets/BasePopup.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>

namespace chatterino {

class QualityPopup : public BasePopup
{
public:
    QualityPopup(const QString &channelURL, QStringList options,
                 QStringList extraArguments = QStringList(),
                 bool streamVLC = false);
    static void showDialog(const QString &channelURL, QStringList options,
                           QStringList extraArguments = QStringList(),
                           bool streamVLC = false);

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

    QString channelURL_;
    QStringList extraArguments_;
    bool streamVLC_;
};

}  // namespace chatterino
