#pragma once

#include <QListWidget>
#include <QListWidgetItem>
#include <QString>

namespace chatterino {

class IntItem : public QListWidgetItem
{
public:
    static constexpr int TYPE_ID = QListWidgetItem::UserType + 101;

    IntItem(int v = 0, QListWidget *parent = nullptr);

    /// setText should not be used, we only store int values in this item
    ///
    /// use setValue instead.
    void setText(const QString &) = delete;

    bool operator<(const QListWidgetItem &other) const override;

    void setValue(int v);
    int getValue() const;

private:
    int value;
};

IntItem *findIntItemInList(QListWidget *list, int value);

}  // namespace chatterino
