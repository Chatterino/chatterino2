#ifndef TEXTINPUTDIALOG_H
#define TEXTINPUTDIALOG_H

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

class TextInputDialog : public QDialog
{
    Q_OBJECT

public:
    TextInputDialog(QWidget *parent = NULL);

    QString
    getText() const
    {
        return lineEdit.text();
    }

    void
    setText(const QString &text)
    {
        lineEdit.setText(text);
    }

private:
    QVBoxLayout vbox;
    QLineEdit lineEdit;
    QHBoxLayout buttonBox;
    QPushButton okButton;
    QPushButton cancelButton;

private slots:
    void okButtonClicked();
    void cancelButtonClicked();
};

#endif  // TEXTINPUTDIALOG_H
