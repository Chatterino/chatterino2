#pragma once

#include <QFont>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringListModel>
#include <QWidget>

namespace chatterino {

class FontFamilyWidget : public QWidget
{
    Q_OBJECT

public:
    FontFamilyWidget(const QFont &startFont, QWidget *parent = nullptr);

    /// Gets the currently selected font family (e.g. "Arial").
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
