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

class FontFamilyWidget : public QWidget
{
    Q_OBJECT

public:
    FontFamilyWidget(const QFont &startFont, QWidget *parent = nullptr);
    QString getSelected() const;

Q_SIGNALS:
    void selectedChanged();

private:
    void setSelected(const QString &family);

    QListView *list;
    QStringListModel *model;
    QSortFilterProxyModel *proxy;
};

}  // namespace chatterino
