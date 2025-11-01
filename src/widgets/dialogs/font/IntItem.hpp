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

class IntItem : public QListWidgetItem
{
public:
    void setText(const QString &) = delete;

    IntItem(int v = 0, QListWidget *parent = nullptr);

    bool operator<(const QListWidgetItem &other) const override;

    void setValue(int v);
    int getValue() const;
    static int typeId();

private:
    int value;
};

IntItem *findIntItemInList(QListWidget *list, int value);

}  // namespace chatterino
