#pragma once

#include "common/SignalVectorModel.hpp"

#include <QObject>

namespace chatterino {

class NotificationController;

class NotificationModel : public SignalVectorModel<QString>
{
    explicit NotificationModel(QObject *parent);

protected:
    // turn a vector item into a model row
    QString getItemFromRow(std::vector<QStandardItem *> &row,
                           const QString &original) override;

    // turns a row in the model into a vector item
    void getRowFromItem(const QString &item,
                        std::vector<QStandardItem *> &row) override;

    friend class NotificationController;
};

}  // namespace chatterino
