#pragma once

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

namespace chatterino
{
    class TextInputDialog : public QDialog
    {
        Q_OBJECT

    public:
        TextInputDialog(QWidget* parent = nullptr);

        QString getText() const;
        void setText(const QString& text);

        void highlightText();

    private:
        QVBoxLayout vbox_;
        QLineEdit lineEdit_;
        QHBoxLayout buttonBox_;
        QPushButton okButton_;
        QPushButton cancelButton_;

    private slots:
        void okButtonClicked();
        void cancelButtonClicked();
    };
}  // namespace chatterino
