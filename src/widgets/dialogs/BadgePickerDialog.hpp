#pragma once

#include "util/DisplayBadge.hpp"

#include <QComboBox>
#include <QDialog>

#include <memory>
#include <optional>

namespace chatterino {

class BadgePickerDialog : public QDialog,
                          public std::enable_shared_from_this<BadgePickerDialog>
{
    using QIconPtr = std::shared_ptr<QIcon>;
    using BadgeOpt = std::optional<DisplayBadge>;

public:
    BadgePickerDialog(QList<DisplayBadge> badges, QWidget *parent = nullptr);

    BadgeOpt getSelection() const
    {
        return this->currentBadge_;
    }

private:
    QComboBox *dropdown_;

    BadgeOpt currentBadge_;
};

}  // namespace chatterino
