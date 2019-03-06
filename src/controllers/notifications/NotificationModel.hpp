#pragma once

#include <QObject>

#include "common/SignalVectorModel.hpp"
#include "controllers/notifications/NotificationController.hpp"

namespace chatterino
{
    class NotificationController;

    class NotificationModel : public SignalVectorModel<QString>
    {
        explicit NotificationModel(QObject* parent);

    protected:
        // turn a vector item into a model row
        virtual QString getItemFromRow(
            std::vector<QStandardItem*>& row, const QString& original) override;

        // turns a row in the model into a vector item
        virtual void getRowFromItem(
            const QString& item, std::vector<QStandardItem*>& row) override;

        friend class NotificationController;
    };

}  // namespace chatterino
