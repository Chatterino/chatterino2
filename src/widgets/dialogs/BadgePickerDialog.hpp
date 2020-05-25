#pragma once

#include "util/DisplayBadge.hpp"

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

namespace chatterino {

class BadgePickerDialog : public QDialog,
                          public std::enable_shared_from_this<BadgePickerDialog>
{
    Q_OBJECT
    using QIconPtr = std::shared_ptr<QIcon>;

public:
    BadgePickerDialog(QList<DisplayBadge> badges, QWidget *parent = nullptr);

    boost::optional<DisplayBadge> getSelection() const;

private:
    void initializeValues();
    QVBoxLayout vbox_;
    QComboBox dropdown_;
    QHBoxLayout buttonBox_;
    QPushButton okButton_;
    QPushButton cancelButton_;
    QList<DisplayBadge> badges_;

private slots:
    void okButtonClicked();
    void cancelButtonClicked();
};

}  // namespace chatterino
