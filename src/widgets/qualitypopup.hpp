#ifndef QUALITYPOPUP_H
#define QUALITYPOPUP_H

#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "basewindow.hpp"
#include "singletons/settingsmanager.hpp"

namespace chatterino {

namespace widgets {

class QualityPopup : public BaseWindow
{
public:
    QualityPopup(const QString &channel, const QString &path, QStringList options);
    static void showDialog(const QString &channel, const QString &path, QStringList options);

private:
    struct {
        QVBoxLayout vbox;
        QComboBox selector;
        QDialogButtonBox buttonBox;
        QPushButton okButton;
        QPushButton cancelButton;
    } ui;

    QString channel;
    QString path;

    void okButtonClicked();
    void cancelButtonClicked();
};

}  // namespace widgets
}  // namespace chatterino

#endif  // QUALITYPOPUP_H
