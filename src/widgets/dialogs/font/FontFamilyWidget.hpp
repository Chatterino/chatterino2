#pragma once

#include <QListView>
#include <QSortFilterProxyModel>
#include <QStringListModel>

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
