#pragma once

#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSpinBox>
#include <QStringListModel>
#include <QToolButton>
#include <QVBoxLayout>

namespace chatterino {

class FontWeightWidget : public QWidget
{
    Q_OBJECT

public:
    FontWeightWidget(const QFont &startFont, QWidget *parent = nullptr);

    void setFamily(const QString &family);
    int getSelected() const;

Q_SIGNALS:
    void selectedChanged();

private:
    QListWidget *list;
};

}  // namespace chatterino
