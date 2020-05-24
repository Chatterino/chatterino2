#pragma once

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

namespace chatterino {

class BadgePickerDialog : public QDialog
{
    Q_OBJECT

public:
    enum Badges {
        Broadcaster = 0,
        Admin = 1,
        Verified = 2,
        Staff = 3,
        VIP = 4,
        Moderator = 5,
        GlobalModerator = 6
    };

    BadgePickerDialog(QWidget *parent = nullptr);

    Badges getSelection() const;

private:
    void initializeValues();
    QVBoxLayout vbox_;
    QComboBox dropdown_;
    QHBoxLayout buttonBox_;
    QPushButton okButton_;
    QPushButton cancelButton_;

private slots:
    void okButtonClicked();
    void cancelButtonClicked();
};

}  // namespace chatterino
