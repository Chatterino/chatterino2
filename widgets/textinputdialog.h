#ifndef TEXTINPUTDIALOG_H
#define TEXTINPUTDIALOG_H

#include <QDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

class TextInputDialog : public QDialog
{
    Q_OBJECT

public:
    TextInputDialog(QWidget *parent = NULL);

    QString getText() const
    {
        return _lineEdit.text();
    }

    void setText(const QString &text)
    {
        _lineEdit.setText(text);
    }

private:
    QVBoxLayout _vbox;
    QLineEdit _lineEdit;
    QHBoxLayout _buttonBox;
    QPushButton _okButton;
    QPushButton _cancelButton;

private slots:
    void okButtonClicked();
    void cancelButtonClicked();
};
}
}

#endif  // TEXTINPUTDIALOG_H
