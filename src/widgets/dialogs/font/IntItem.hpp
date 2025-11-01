#pragma once
#include <QListWidget>

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
