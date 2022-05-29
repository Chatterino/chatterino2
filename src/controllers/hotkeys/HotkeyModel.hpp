#pragma once

#include "common/SignalVectorModel.hpp"
#include "controllers/hotkeys/Hotkey.hpp"
#include "util/QStringHash.hpp"

#include <unordered_map>

namespace chatterino {

class HotkeyController;

class HotkeyModel : public SignalVectorModel<std::shared_ptr<Hotkey>>
{
public:
    HotkeyModel(QObject *parent);

protected:
    // turn a vector item into a model row
    virtual std::shared_ptr<Hotkey> getItemFromRow(
        std::vector<QStandardItem *> &row,
        const std::shared_ptr<Hotkey> &original) override;

    // turns a row in the model into a vector item
    virtual void getRowFromItem(const std::shared_ptr<Hotkey> &item,
                                std::vector<QStandardItem *> &row) override;

    virtual int beforeInsert(const std::shared_ptr<Hotkey> &item,
                             std::vector<QStandardItem *> &row,
                             int proposedIndex) override;

    virtual void afterRemoved(const std::shared_ptr<Hotkey> &item,
                              std::vector<QStandardItem *> &row,
                              int index) override;

    friend class HotkeyController;

private:
    std::tuple<int, int> getCurrentAndNextCategoryModelIndex(
        const QString &category) const;

    std::unordered_map<QString, int> categoryCount_;
};

}  // namespace chatterino
