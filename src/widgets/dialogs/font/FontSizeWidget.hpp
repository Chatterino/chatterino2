#pragma once
#include "widgets/dialogs/font/IntItem.hpp"

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

class FontSizeWidget : public QWidget
{
    Q_OBJECT

public:
    FontSizeWidget(const QFont &startFont, QWidget *parent = nullptr);
    int getSelected() const;

Q_SIGNALS:
    void selectedChanged();

private:
    void setListSelected(int size);

    IntItem *customItem;  // displays the value from `edit`
    QListWidget *list;
    QSpinBox *edit;
};

}  // namespace chatterino
